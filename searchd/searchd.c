/*
 * Phosh Search Daemon
 *
 * Copyright (C) 2019 Zander Brown
 *
 * Based on gnome-shell's original js implementation
 * https://gitlab.gnome.org/GNOME/gnome-shell/blob/2d2824b947754abf0ddadd9c1ba9b9f16b0745d3/js/ui/search.js
 * https://gitlab.gnome.org/GNOME/gnome-shell/blob/0a7e717e0e125248bace65e170a95ae12e3cdf38/js/ui/remoteSearch.js
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "phosh-searchd.h"
#include "searchd.h"
#include "search-provider.h"

#include <search-source.h>
#include <search-result-meta.h>

#define GROUP_NAME "Shell Search Provider"
#define SEARCH_PROVIDERS_SCHEMA "org.gnome.desktop.search-providers"


typedef struct _PhoshSearchApplicationPrivate PhoshSearchApplicationPrivate;
struct _PhoshSearchApplicationPrivate {
  PhoshSearchDBusSearch *object;

  GSettings *settings;

  // element-type: Phosh.SearchSource
  GList        *sources;
  // element-type: Phosh.SearchProvider
  GHashTable   *providers;

  // key: char * (object path), value: char ** (results)
  GHashTable   *last_results;
  gboolean      doing_subsearch;

  char         *query;
  GStrv         query_parts;

  GCancellable *cancellable;

  gulong        search_timeout;

  GRegex       *splitter;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshSearchApplication, phosh_search_application, G_TYPE_APPLICATION)


static void
phosh_search_application_finalize (GObject *object)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (object);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);

  g_clear_object (&priv->object);

  g_cancellable_cancel (priv->cancellable);

  g_clear_object (&priv->cancellable);
  g_clear_object (&priv->settings);
  g_clear_pointer (&priv->last_results, g_hash_table_destroy);

  g_clear_pointer (&priv->query, g_free);
  g_clear_pointer (&priv->query_parts, g_strfreev);

  g_list_free_full (priv->sources, (GDestroyNotify) phosh_search_source_unref);
  g_clear_pointer (&priv->providers, g_hash_table_destroy);

  G_OBJECT_CLASS (phosh_search_application_parent_class)->finalize (object);
}


static gboolean
phosh_search_application_dbus_register (GApplication     *app,
                                        GDBusConnection  *connection,
                                        const gchar      *object_path,
                                        GError          **error)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (app);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);

  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->object),
                                         connection,
                                         object_path,
                                         error)) {
    g_clear_object (&priv->object);

    return FALSE;
  }

  return G_APPLICATION_CLASS (phosh_search_application_parent_class)->dbus_register (app,
                                                                                     connection,
                                                                                     object_path,
                                                                                     error);
}


static void
phosh_search_application_dbus_unregister (GApplication    *app,
                                          GDBusConnection *connection,
                                          const gchar     *object_path)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (app);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);

  if (priv->object) {
    g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (priv->object));

    g_clear_object (&priv->object);
  }

  G_APPLICATION_CLASS (phosh_search_application_parent_class)->dbus_unregister (app,
                                                                                connection,
                                                                                object_path);
}


static void
phosh_search_application_class_init (PhoshSearchApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->finalize = phosh_search_application_finalize;

  app_class->dbus_register = phosh_search_application_dbus_register;
  app_class->dbus_unregister = phosh_search_application_dbus_unregister;
}


static gboolean
launch_source (PhoshSearchDBusSearch *interface,
               GDBusMethodInvocation *invocation,
               const gchar           *source_id,
               guint                  timestamp,
               gpointer               user_data)
{
  phosh_search_dbus_search_complete_launch_source (interface, invocation);

  return TRUE;
}


static gboolean
activate_result (PhoshSearchDBusSearch *interface,
                 GDBusMethodInvocation *invocation,
                 const gchar           *source_id,
                 const gchar           *result_id,
                 guint                  timestamp,
                 gpointer               user_data)
{
  g_message ("activate %s - %s at %i", source_id, result_id, timestamp);

  phosh_search_dbus_search_complete_activate_result (interface, invocation);

  return TRUE;
}


static gboolean
get_sources (PhoshSearchDBusSearch *interface,
             GDBusMethodInvocation *invocation,
             gpointer               user_data)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (user_data);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  GVariantBuilder builder;
  g_autoptr (GVariant) result = NULL;
  GList *list;

  list = priv->sources;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(ssu)"));

  while (list) {
    g_variant_builder_add_value (&builder, phosh_search_source_serialise (list->data));

    list = g_list_next (list);
  }

  result = g_variant_builder_end (&builder);

  phosh_search_dbus_search_complete_get_sources (interface,
                                                 invocation,
                                                 g_variant_ref (result));

  return TRUE;
}


static void
got_metas (GObject      *source,
           GAsyncResult *res,
           gpointer      user_data)
{
  g_autoptr (GError) error = NULL;
  GPtrArray *metas = phosh_search_provider_get_result_meta_finish (PHOSH_SEARCH_PROVIDER (source), res, &error);
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (user_data);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  GVariantBuilder builder;
  g_autoptr (GVariant) result = NULL;
  char *bus_path;

  g_object_get (source, "bus-path", &bus_path, NULL);

  if (error) {
    g_critical ("Failed to load results %s", error->message);
    return;
  }

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("aa{sv}"));

  for (int i = 0; i < metas->len; i++) {
    g_variant_builder_add_value (&builder,
                                 phosh_search_result_meta_serialise (g_ptr_array_index (metas, i)));
  }

  result = g_variant_builder_end (&builder);

  phosh_search_dbus_search_emit_source_results_changed (priv->object,
                                                        bus_path,
                                                        g_variant_ref (result));
}


struct GotResultsData {
  gboolean initial;
  PhoshSearchApplication *self;
};


static void
got_results (GObject      *source,
             GAsyncResult *res,
             gpointer      user_data)
{
  PhoshSearchApplicationPrivate *priv;
  struct GotResultsData *data = user_data;
  g_autoptr (GError) error = NULL;
  GStrv results = NULL;
  g_autoptr (GPtrArray) sub_res = NULL;
  char *bus_path = NULL;
  GStrv sub_res_strv = NULL;

  priv = phosh_search_application_get_instance_private (data->self);

  g_object_get (source, "bus-path", &bus_path, NULL);

  if (data->initial) {
    results = phosh_search_provider_get_initial_finish (PHOSH_SEARCH_PROVIDER (source), res, &error);
  } else {
    results = phosh_search_provider_get_subsearch_finish (PHOSH_SEARCH_PROVIDER (source), res, &error);
  }

  if (error) {
    g_warning ("[%s]: %s", bus_path, error->message);
  } else if (results) {
    sub_res = phosh_search_provider_limit_results (results, 5);

    sub_res_strv = g_new (char *, sub_res->len + 1);

    for (int i = 0; i < sub_res->len; i++) {
      sub_res_strv[i] = g_ptr_array_index (sub_res, i);
    }
    sub_res_strv[sub_res->len] = NULL;

    phosh_search_provider_get_result_meta (PHOSH_SEARCH_PROVIDER (source),
                                           sub_res_strv,
                                           got_metas,
                                           data->self);

    g_hash_table_insert (priv->last_results, bus_path, results);
  }

  g_object_unref (data->self);
  g_free (data);
}


static void
search (PhoshSearchApplication *self)
{
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, priv->providers);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    PhoshSearchProvider *provider = PHOSH_SEARCH_PROVIDER (value);
    char *bus_path;
    struct GotResultsData *data;

    g_object_get (provider, "bus-path", &bus_path, NULL);

    if (!phosh_search_provider_get_ready (provider)) {
      g_warning ("[%s]: not ready", bus_path);
      continue;
    }

    data = g_new (struct GotResultsData, 1);
    data->self = g_object_ref (self);

    if (priv->doing_subsearch &&
        g_hash_table_contains (priv->last_results, bus_path)) {
      data->initial = FALSE;
      phosh_search_provider_get_subsearch (provider,
                                           g_hash_table_lookup (priv->last_results, bus_path),
                                           (const char * const*) priv->query_parts,
                                           got_results,
                                           data);
    } else {
      data->initial = TRUE;

      phosh_search_provider_get_initial (provider,
                                         (const char * const*) priv->query_parts,
                                         got_results,
                                         data);
    }
  }

  g_hash_table_remove_all (priv->last_results);

  if (priv->search_timeout != 0) {
    g_source_remove (priv->search_timeout);
    priv->search_timeout = 0;
  }
}


static gboolean
search_timeout (gpointer user_data)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (user_data);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);

  priv->search_timeout = 0;

  search (self);

  return G_SOURCE_REMOVE;
}


// Taken from gstrfuncs.c in GLib 2.60
static gboolean
strv_equal (const gchar * const *strv1,
            const gchar * const *strv2)
{
  g_return_val_if_fail (strv1 != NULL, FALSE);
  g_return_val_if_fail (strv2 != NULL, FALSE);

  if (strv1 == strv2)
    return TRUE;

  for (; *strv1 != NULL && *strv2 != NULL; strv1++, strv2++)
    {
      if (!g_str_equal (*strv1, *strv2))
        return FALSE;
    }

  return (*strv1 == NULL && *strv2 == NULL);
}


static gboolean
query (PhoshSearchDBusSearch *interface,
       GDBusMethodInvocation *invocation,
       const char            *query,
       gpointer               user_data)
{
  PhoshSearchApplication *self = PHOSH_SEARCH_APPLICATION (user_data);
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  g_autofree char *striped = NULL;
  g_auto (GStrv) parts = NULL;
  int len = 0;

  striped = g_strstrip (g_strdup (query));
  parts = g_regex_split (priv->splitter, striped, 0);

  len = parts ? g_strv_length (parts) : 0;

  if (priv->query_parts &&
      strv_equal ((const char *const *) priv->query_parts,
                  (const char *const *) parts)) {
    phosh_search_dbus_search_complete_query (interface, invocation, FALSE);

    return TRUE;
  }

  g_cancellable_cancel (priv->cancellable);
  g_cancellable_reset (priv->cancellable);

  if (len == 0) {
    g_clear_pointer (&priv->query, g_free);
    g_clear_pointer (&priv->query_parts, g_strfreev);

    priv->doing_subsearch = FALSE;

    phosh_search_dbus_search_complete_query (interface, invocation, FALSE);

    return TRUE;
  }

  if (priv->query != NULL) {
    priv->doing_subsearch = g_str_has_prefix (query, priv->query);
  } else {
    priv->doing_subsearch = FALSE;
  }

  g_clear_pointer (&priv->query_parts, g_strfreev);
  priv->query_parts = g_strdupv (parts);
  priv->query = g_strdup (query);

  if (priv->search_timeout == 0) {
    priv->search_timeout = g_timeout_add (150, search_timeout, self);
  }

  phosh_search_dbus_search_complete_query (interface, invocation, TRUE);

  return TRUE;
}


// Sort algorithm taken straight from remoteSearch.js, comments and all
static int
sort_sources (gconstpointer a,
              gconstpointer b,
              gpointer      user_data)
{
  GAppInfo *app_a = NULL;
  GAppInfo *app_b = NULL;
  const char *app_id_a = NULL;
  const char *app_id_b = NULL;
  GStrv order = user_data;
  int idx_a = -1;
  int idx_b = -1;
  int i = 0;

  g_return_val_if_fail (a != NULL, -1);
  g_return_val_if_fail (b != NULL, -1);

  app_a = phosh_search_source_get_app_info ((PhoshSearchSource *) a);
  app_b = phosh_search_source_get_app_info ((PhoshSearchSource *) b);

  g_return_val_if_fail (G_IS_APP_INFO (app_a), -1);
  g_return_val_if_fail (G_IS_APP_INFO (app_b), -1);

  app_id_a = g_app_info_get_id (app_a);
  app_id_b = g_app_info_get_id (app_b);

  while ((order[i])) {
    if (idx_a == -1 && g_strcmp0 (order[i], app_id_a) == 0) {
      idx_a = i;
    }

    if (idx_b == -1 && g_strcmp0 (order[i], app_id_b) == 0) {
      idx_b = i;
    }

    if (idx_a != -1 && idx_b != -1) {
      break;
    }

    i++;
  }

  // if no provider is found in the order, use alphabetical order
  if ((idx_a == -1) && (idx_b == -1)) {
    return g_utf8_collate (g_app_info_get_name (app_a),
                           g_app_info_get_name (app_b));
  }

  // if providerA isn't found, it's sorted after providerB
  if (idx_a == -1)
    return 1;

  // if providerB isn't found, it's sorted after providerA
  if (idx_b == -1)
    return -1;

  // finally, if both providers are found, return their order in the list
  return (idx_a - idx_b);
}


// Don't rely on settings or key, they are null when called for an app change
static void
reload_providers (GSettings              *obj,
                  char                   *key,
                  PhoshSearchApplication *self)
{
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  const char *const *data_dirs = g_get_system_data_dirs ();
  const char *data_dir = NULL;
  g_autolist (PhoshSearchSource) sources = NULL;
  // These two skip the normal sorting
  g_autoptr (PhoshSearchSource) apps = NULL;
  g_autoptr (PhoshSearchSource) settings = NULL;
  g_auto (GStrv) enabled = NULL;
  g_auto (GStrv) disabled = NULL;
  g_auto (GStrv) sort_order = NULL;
  GList *list;
  int i = 0;

  if (g_settings_get_boolean (priv->settings, "disable-external")) {
    g_list_free_full (priv->sources, (GDestroyNotify) phosh_search_source_unref);
  }

  enabled = g_settings_get_strv (priv->settings, "enabled");
  disabled = g_settings_get_strv (priv->settings, "disabled");
  sort_order = g_settings_get_strv (priv->settings, "sort-order");

  while ((data_dir = data_dirs[i])) {
    g_autofree char *dir = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GDir) contents = NULL;
    const char* name = NULL;

    i++;

    dir = g_build_filename (data_dir, "gnome-shell", "search-providers", NULL);
    contents = g_dir_open (dir, 0, &error);

    if (error) {
      if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
        g_warning ("Can't look for in %s: %s", dir, error->message);
      }
      g_clear_error (&error);
      continue;
    }

    while ((name = g_dir_read_name (contents))) {
      g_autofree char *provider = NULL;
      g_autofree char *bus_path = NULL;
      g_autofree char *bus_name = NULL;
      g_autofree char *desktop_id = NULL;
      g_autoptr (GKeyFile) data = NULL;
      g_autoptr (PhoshSearchProvider) provider_object = NULL;
      g_autoptr (PhoshSearchSource) source = NULL;
      int version = 0;
      gboolean autostart = TRUE;
      gboolean autostart_tmp = FALSE;
      gboolean default_disabled = FALSE;
      gboolean default_disabled_tmp = FALSE;

      provider = g_build_filename (dir, name, NULL);
      data = g_key_file_new ();

      g_key_file_load_from_file (data, provider, G_KEY_FILE_NONE, &error);

      if (error) {
        g_warning ("Can't read %s: %s", provider, error->message);
        g_clear_error (&error);
        continue;
      }

      if (!g_key_file_has_group (data, GROUP_NAME)) {
        g_warning ("%s doesn't define a search provider", provider);
        continue;
      }


      version = g_key_file_get_integer (data, GROUP_NAME, "Version", &error);

      if (error) {
        g_warning ("Failed to fetch provider version %s: %s", provider, error->message);
        g_clear_error (&error);
        continue;
      }

      if (version < 2) {
        g_warning ("Provider %s implements version %i but we only support version 2 and up", provider, version);
        continue;
      }


      desktop_id = g_key_file_get_string (data, GROUP_NAME, "DesktopId", &error);

      if (error) {
        g_warning ("Failed to fetch provider desktop id %s: %s", provider, error->message);
        g_clear_error (&error);
        continue;
      }

      if (!desktop_id) {
        g_warning ("Provider %s doesn't specify a desktop id", provider);
        continue;
      }


      bus_name = g_key_file_get_string (data, GROUP_NAME, "BusName", &error);

      if (error) {
        g_warning ("Failed to fetch provider bus name %s: %s", provider, error->message);
        g_clear_error (&error);
        continue;
      }

      if (!bus_name) {
        g_warning ("Provider %s doesn't specify a bus name", provider);
        continue;
      }


      bus_path = g_key_file_get_string (data, GROUP_NAME, "ObjectPath", &error);

      if (error) {
        g_warning ("Failed to fetch provider bus path %s: %s", provider, error->message);
        g_clear_error (&error);
        continue;
      }

      if (!bus_path) {
        g_warning ("Provider %s doesn't specify a bus path", provider);
        continue;
      }

      if (g_hash_table_contains (priv->providers, bus_path)) {
        g_debug ("We already have a provider for %s, ignoring %s", bus_path, provider);
        continue;
      }


      autostart_tmp = g_key_file_get_boolean (data, GROUP_NAME, "AutoStart", &error);

      if (G_LIKELY (error)) {
        g_clear_error (&error);
      } else {
        autostart = autostart_tmp;
      }


      default_disabled_tmp = g_key_file_get_boolean (data, GROUP_NAME, "DefaultDisabled", &error);

      if (G_LIKELY (error)) {
        g_clear_error (&error);
      } else {
        default_disabled = default_disabled_tmp;
      }

      if (!default_disabled) {
        if (g_strv_contains ((const char * const*) disabled, desktop_id)) {
          g_debug ("Provider %s has been disabled", provider);
          continue;
        }
      } else {
        if (!g_strv_contains ((const char * const*) enabled, desktop_id)) {
          g_debug ("Provider %s hasn't been enabled", provider);
          continue;
        }
      }

      provider_object = phosh_search_provider_new (desktop_id,
                                                   priv->cancellable,
                                                   bus_path,
                                                   bus_name,
                                                   autostart,
                                                   default_disabled);

      if (G_UNLIKELY (g_str_equal (desktop_id, "sm.puri.Phosh.AppSearch.desktop"))) {
        apps = phosh_search_source_new (bus_path,
                                        G_APP_INFO (g_desktop_app_info_new (desktop_id)));
      } else if (G_UNLIKELY (g_str_equal (desktop_id, "gnome-control-center.desktop"))) {
        settings = phosh_search_source_new (bus_path,
                                            G_APP_INFO (g_desktop_app_info_new (desktop_id)));
      } else {
        source = phosh_search_source_new (bus_path,
                                          G_APP_INFO (g_desktop_app_info_new (desktop_id)));

        sources = g_list_prepend (sources, phosh_search_source_ref (source));
      }

      g_hash_table_insert (priv->providers,
                           g_strdup (bus_path),
                           g_object_ref (provider_object));
    }
  }

  sources = g_list_sort_with_data (sources, sort_sources, sort_order);

  if (settings) {
    sources = g_list_prepend (sources, phosh_search_source_ref (settings));
  }

  if (apps) {
    sources = g_list_prepend (sources, phosh_search_source_ref (apps));
  }

  list = sources;
  i = 0;

  while (list) {
    phosh_search_source_set_position (list->data, i);

    i++;

    list = g_list_next (list);
  }

  g_list_free_full (priv->sources, (GDestroyNotify) phosh_search_source_unref);
  priv->sources = g_list_copy_deep (sources, (GCopyFunc) phosh_search_source_ref, NULL);
}


static void
reload_providers_apps_changed (GAppInfoMonitor        *monitor,
                               PhoshSearchApplication *self)
{
  reload_providers (NULL, NULL, self);
}


static void
phosh_search_application_init (PhoshSearchApplication *self)
{
  PhoshSearchApplicationPrivate *priv = phosh_search_application_get_instance_private (self);
  g_autoptr (GError) error = NULL;

  priv->doing_subsearch = FALSE;
  priv->search_timeout = 0;

  priv->splitter = g_regex_new ("\\s+",
                                G_REGEX_CASELESS | G_REGEX_MULTILINE,
                                0,
                                &error);

  if (error) {
    g_error ("Bad Regex: %s", error->message);
  }

  priv->last_results = g_hash_table_new_full (g_str_hash,
                                              g_str_equal,
                                              g_free,
                                              (GDestroyNotify) g_strfreev);

  priv->sources = NULL;
  priv->providers = g_hash_table_new_full (g_str_hash,
                                           g_str_equal,
                                           g_free,
                                           (GDestroyNotify) g_object_unref);

  priv->cancellable = g_cancellable_new ();

  priv->settings = g_settings_new (SEARCH_PROVIDERS_SCHEMA);
  g_signal_connect (priv->settings, "changed::disabled",
                    G_CALLBACK (reload_providers), self);
  g_signal_connect (priv->settings, "changed::enabled",
                    G_CALLBACK (reload_providers), self);
  g_signal_connect (priv->settings, "changed::disable-external",
                    G_CALLBACK (reload_providers), self);
  g_signal_connect (priv->settings, "changed::sort-order",
                    G_CALLBACK (reload_providers), self);

  g_signal_connect (g_app_info_monitor_get (), "changed",
                    G_CALLBACK (reload_providers_apps_changed), self);


  priv->object = phosh_search_dbus_search_skeleton_new ();
  g_signal_connect (priv->object,
                    "handle-launch-source",
                    G_CALLBACK (launch_source),
                    self);
  g_signal_connect (priv->object,
                    "handle-activate-result",
                    G_CALLBACK (activate_result),
                    self);
  g_signal_connect (priv->object,
                    "handle-get-sources",
                    G_CALLBACK (get_sources),
                    self);
  g_signal_connect (priv->object,
                    "handle-query",
                    G_CALLBACK (query),
                    self);

  reload_providers (NULL, NULL, self);

  g_application_hold (G_APPLICATION (self));
}


int
main (int argc, char **argv)
{
  g_autoptr (GApplication) app = NULL;

  app = g_object_new (PHOSH_TYPE_SEARCH_APPLICATION,
                      "application-id", "sm.puri.Phosh.Search",
                      "flags", G_APPLICATION_IS_SERVICE,
                      NULL);

  return g_application_run (app, argc, argv);
}

/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * Inspired by search.js / remoteSearch.js:
 * https://gitlab.gnome.org/GNOME/gnome-shell/blob/2d2824b947754abf0ddadd9c1ba9b9f16b0745d3/js/ui/search.js
 * https://gitlab.gnome.org/GNOME/gnome-shell/blob/0a7e717e0e125248bace65e170a95ae12e3cdf38/js/ui/remoteSearch.js
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#define G_LOG_DOMAIN "phosh-search"

#include "search.h"
#include "search-provider.h"
#include "app-list-model.h"

#define GROUP_NAME "Shell Search Provider"
#define SEARCH_PROVIDERS_SCHEMA "org.gnome.desktop.search-providers"

typedef struct _PhoshSearchPrivate PhoshSearchPrivate;
struct _PhoshSearchPrivate {
  GSettings *settings;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshSearch, phosh_search, G_TYPE_OBJECT)

static void
phosh_search_finalize (GObject *object)
{
  PhoshSearch *self = PHOSH_SEARCH (object);
  PhoshSearchPrivate *priv = phosh_search_get_instance_private (self);

  g_clear_object (&priv->settings);
}

static void
phosh_search_class_init (PhoshSearchClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phosh_search_finalize;
}

// Sort algorithm taken straight from remoteSearch.js, comments and all
static int
sort_providers (gconstpointer a,
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

  g_return_val_if_fail (PHOSH_IS_SEARCH_PROVIDER ((gpointer) a), -1);
  g_return_val_if_fail (PHOSH_IS_SEARCH_PROVIDER ((gpointer) b), -1);

  g_object_get ((gpointer) a, "app-info", &app_a, NULL);
  g_object_get ((gpointer) b, "app-info", &app_b, NULL);

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
reload_providers (GSettings   *settings,
                  char        *key,
                  PhoshSearch *self)
{
  PhoshSearchPrivate *priv = phosh_search_get_instance_private (self);
  const char *const *data_dirs = g_get_system_data_dirs ();
  const char *data_dir = NULL;
  g_autoptr (GHashTable) provider_set = NULL;
  g_autolist (PhoshSearchProvider) providers = NULL;
  g_auto (GStrv) enabled = NULL;
  g_auto (GStrv) disabled = NULL;
  g_auto (GStrv) sort_order = NULL;
  int i = 0;

  if (g_settings_get_boolean (priv->settings, "disable-external")) {
    // TODO: Clear all
    return;
  }

  enabled = g_settings_get_strv (priv->settings, "enabled");
  disabled = g_settings_get_strv (priv->settings, "disabled");
  sort_order = g_settings_get_strv (priv->settings, "sort-order");

  provider_set = g_hash_table_new_full (g_str_hash,
                                        g_str_equal,
                                        g_free,
                                        NULL);


  while ((data_dir = data_dirs[i])) {
    g_autofree char *dir = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GDir) list = NULL;
    const char* name = NULL;

    i++;

    dir = g_build_filename (data_dir, "gnome-shell", "search-providers", NULL);
    list = g_dir_open (dir, 0, &error);

    if (error) {
      if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
        g_warning ("Can't look for in %s: %s", dir, error->message);
      }
      g_clear_error (&error);
      continue;
    }

    while ((name = g_dir_read_name (list))) {
      g_autofree char *provider = NULL;
      g_autofree char *bus_path = NULL;
      g_autofree char *bus_name = NULL;
      g_autofree char *desktop_id = NULL;
      g_autoptr (GKeyFile) data = NULL;
      g_autoptr (PhoshSearchProvider) provider_object = NULL;
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

      if (g_hash_table_contains (provider_set, bus_path)) {
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
                                                   bus_path,
                                                   bus_name,
                                                   autostart,
                                                   default_disabled);

      providers = g_list_prepend (providers, g_object_ref (provider_object));

      g_hash_table_add (provider_set, g_strdup (bus_path));
    }
  }

  providers = g_list_sort_with_data (providers, sort_providers, sort_order);

  providers = g_list_copy_deep (providers, (GCopyFunc) g_object_ref, NULL);

  for (GList *l = providers; l; l = l->next) {
    GAppInfo *info = NULL;

    g_object_get (l->data, "app-info", &info, NULL);

    g_print ("%25s %s\n", g_app_info_get_name (info), g_app_info_get_id (info));
  }
}

static void
reload_providers_apps_changed (GListModel  *list,
                               guint        position,
                               guint        removed,
                               guint        added,
                               PhoshSearch *self)
{
  reload_providers (NULL, NULL, self);
}

static void
phosh_search_init (PhoshSearch *self)
{
  PhoshSearchPrivate *priv = phosh_search_get_instance_private (self);

  priv->settings = g_settings_new (SEARCH_PROVIDERS_SCHEMA);
  g_signal_connect (priv->settings, "changed::disabled",
                    G_CALLBACK (reload_providers), self);
  g_signal_connect (priv->settings, "changed::enabled",
                    G_CALLBACK (reload_providers), self);
  g_signal_connect (priv->settings, "changed::disable-external",
                    G_CALLBACK (reload_providers), self);
  g_signal_connect (priv->settings, "changed::sort-order",
                    G_CALLBACK (reload_providers), self);

  g_signal_connect (phosh_app_list_model_get_default (), "items-changed",
                    G_CALLBACK (reload_providers_apps_changed), self);

  reload_providers (NULL, NULL, self);
}

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
  // element-type: Phosh.SearchProvider
  GList *providers;
  // key: char * (object path), value: char ** (results)
  GHashTable *last_results;
  GStrv terms;
  GCancellable *cancellable;
  gboolean doing_subsearch;
  gulong search_timeout;
  GRegex *highlight;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshSearch, phosh_search, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_TERMS,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };

enum {
  RESET,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

static void
phosh_search_finalize (GObject *object)
{
  PhoshSearch *self = PHOSH_SEARCH (object);
  PhoshSearchPrivate *priv = phosh_search_get_instance_private (self);

  g_cancellable_cancel (priv->cancellable);

  g_clear_object (&priv->cancellable);
  g_clear_object (&priv->settings);
  g_clear_pointer (&priv->last_results, g_hash_table_destroy);
  g_clear_pointer (&priv->terms, g_strfreev);
  g_clear_pointer (&priv->highlight, g_regex_unref);
  g_list_free_full (priv->providers, g_object_unref);
}


static void
phosh_search_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  PhoshSearch *self = PHOSH_SEARCH (object);

  switch (property_id) {
    case PROP_TERMS:
      phosh_search_set_terms (self, g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
phosh_search_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  PhoshSearch *self = PHOSH_SEARCH (object);
  PhoshSearchPrivate *priv = phosh_search_get_instance_private (self);

  switch (property_id) {
    case PROP_TERMS:
      g_value_set_boxed (value, priv->terms);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
phosh_search_class_init (PhoshSearchClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phosh_search_finalize;
  object_class->set_property = phosh_search_set_property;
  object_class->get_property = phosh_search_get_property;

  pspecs[PROP_TERMS] =
    g_param_spec_boxed ("terms", "Terms", "Search terms",
                        G_TYPE_STRV,
                        G_PARAM_READWRITE);


  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  signals[RESET] = g_signal_new ("reset",
                                 G_TYPE_FROM_CLASS (klass),
                                 G_SIGNAL_RUN_LAST,
                                 0, NULL, NULL, NULL,
                                 G_TYPE_NONE, 0);
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
    g_list_free_full (priv->providers, g_object_unref);
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
                                                   priv->cancellable,
                                                   bus_path,
                                                   bus_name,
                                                   autostart,
                                                   default_disabled);

      providers = g_list_prepend (providers, g_object_ref (provider_object));

      g_hash_table_add (provider_set, g_strdup (bus_path));
    }
  }

  providers = g_list_sort_with_data (providers, sort_providers, sort_order);

  g_list_free_full (priv->providers, g_object_unref);
  priv->providers = g_list_copy_deep (providers, (GCopyFunc) g_object_ref, NULL);
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

  priv->doing_subsearch = FALSE;
  priv->search_timeout = 0;
  priv->highlight = NULL;

  priv->last_results = g_hash_table_new_full (g_str_hash,
                                              g_str_equal,
                                              g_free,
                                              (GDestroyNotify) g_strfreev);

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

  g_signal_connect (phosh_app_list_model_get_default (), "items-changed",
                    G_CALLBACK (reload_providers_apps_changed), self);

  reload_providers (NULL, NULL, self);
}

static void
got_metas (GObject      *source,
           GAsyncResult *res,
           gpointer      user_data)
{
  g_autoptr (GError) error = NULL;
  GPtrArray *metas = phosh_search_provider_get_result_meta_finish (PHOSH_SEARCH_PROVIDER (source), res, &error);
  PhoshSearch *self = user_data;
  char *bus_path;

  g_object_get (source, "bus-path", &bus_path, NULL);

  g_print ("%s\n", bus_path);
  for (int i = 0; i < metas->len; i++) {
    PhoshSearchProviderResultMeta *result = g_ptr_array_index (metas, i);

    g_print (" - %s\n", phosh_search_provider_result_meta_get_id (result));
    g_print ("   name: %s\n", phosh_search_markup_string (self, phosh_search_provider_result_meta_get_name (result)));
    g_print ("   desc: %s\n", phosh_search_markup_string (self, phosh_search_provider_result_meta_get_description (result)));
    if (phosh_search_provider_result_meta_get_clipboard_text (result)) {
      g_print ("   cp-t: %s\n", phosh_search_provider_result_meta_get_clipboard_text (result));
    }
  }
  g_print ("\n");
}

struct GotResultsData {
  gboolean initial;
  PhoshSearch *self;
};

static void
got_results (GObject      *source,
             GAsyncResult *res,
             gpointer      user_data)
{
  PhoshSearchPrivate *priv;
  struct GotResultsData *data = user_data;
  g_autoptr (GError) error = NULL;
  GStrv results = NULL;
  g_autoptr (GPtrArray) sub_res = NULL;
  char *bus_path = NULL;
  GStrv sub_res_strv = NULL;

  priv = phosh_search_get_instance_private (data->self);

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
search (PhoshSearch *self)
{
  PhoshSearchPrivate *priv = phosh_search_get_instance_private (self);

  for (GList *l = priv->providers; l != NULL; l = g_list_next (l)) {
    PhoshSearchProvider *provider = PHOSH_SEARCH_PROVIDER (l->data);
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
                                           (const char * const*) priv->terms,
                                           got_results,
                                           data);
    } else {
      data->initial = TRUE;
      phosh_search_provider_get_initial (provider,
                                         (const char * const*) priv->terms,
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
  PhoshSearch *self = PHOSH_SEARCH (user_data);
  PhoshSearchPrivate *priv = phosh_search_get_instance_private (self);

  priv->search_timeout = 0;

  search (self);

  return G_SOURCE_REMOVE;
}

void
phosh_search_set_terms (PhoshSearch *self,
                        GStrv        terms)
{
  PhoshSearchPrivate *priv = phosh_search_get_instance_private (self);
  g_autofree char *old_joined = NULL;
  g_autofree char *new_joined = NULL;
  g_autofree char *regex_terms = NULL;
  g_autofree char *regex = NULL;
  g_auto (GStrv) escaped = NULL;
  g_autoptr (GError) error = NULL;
  int i = 0;
  int len = 0;

  if (priv->terms &&
      g_strv_equal ((const char *const *) priv->terms,
                    (const char *const *) terms)) {
    return;
  }

  g_cancellable_cancel (priv->cancellable);
  g_cancellable_reset (priv->cancellable);

  len = terms ? g_strv_length (terms) : 0;

  if (len == 0) {
    g_signal_emit (self, signals[RESET], 0);
    g_clear_pointer (&priv->terms, g_strfreev);
    priv->doing_subsearch = FALSE;
    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_TERMS]);
    return;
  }

  if (priv->terms != NULL) {
    old_joined = g_strjoinv (" ", priv->terms);
    new_joined = g_strjoinv (" ", terms);

    priv->doing_subsearch = g_str_has_prefix (new_joined, old_joined);
  } else {
    priv->doing_subsearch = FALSE;
  }

  escaped = g_new (char *, len + 1);

  while (terms[i]) {
    escaped[i] = g_regex_escape_string (terms[i], -1);

    i++;
  }
  escaped[len] = NULL;
  regex_terms = g_strjoinv ("|", escaped);
  regex = g_strconcat ("(", regex_terms, ")", NULL);
  priv->highlight = g_regex_new (regex, G_REGEX_CASELESS | G_REGEX_MULTILINE, 0, &error);

  if (error) {
    g_warning ("Unable to prepare highlighter: %s", error->message);
    g_clear_error (&error);
    priv->highlight = NULL;
  }

  g_clear_pointer (&priv->terms, g_strfreev);
  priv->terms = g_boxed_copy (G_TYPE_STRV, terms);
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_TERMS]);

  if (priv->search_timeout == 0) {
    priv->search_timeout = g_timeout_add (150, search_timeout, self);
  }
}

const char *
phosh_search_markup_string (PhoshSearch *self,
                            const char  *str)
{
  PhoshSearchPrivate *priv = phosh_search_get_instance_private (self);
  g_autoptr (GError) error = NULL;
  char *res = NULL;

  if (str == NULL) {
    return NULL;
  }

  res = g_regex_replace (priv->highlight,
                         str,
                         -1,
                         0,
                         "<b>\\1</b>",
                         0,
                         &error);

  if (error) {
    return g_strdup (str);
  }

  return res;
}

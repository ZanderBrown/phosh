/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * Inspired by search.js / remoteSearch.js:
 * https://gitlab.gnome.org/GNOME/gnome-shell/blob/2d2824b947754abf0ddadd9c1ba9b9f16b0745d3/js/ui/search.js
 * https://gitlab.gnome.org/GNOME/gnome-shell/blob/0a7e717e0e125248bace65e170a95ae12e3cdf38/js/ui/remoteSearch.js
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#define G_LOG_DOMAIN "phosh-search-provider"

#include "search-provider.h"
#include "dbus/gnome-shell-search-provider.h"

typedef struct _PhoshSearchProviderPrivate PhoshSearchProviderPrivate;
struct _PhoshSearchProviderPrivate {
  GAppInfo                 *info;
  PhoshDBusSearchProvider2 *proxy;
  GCancellable             *cancellable;
  GDBusProxyFlags           proxy_flags;
  char                     *bus_name;
  char                     *bus_path;
  gboolean                  autostart;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshSearchProvider, phosh_search_provider, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_APP_INFO,
  PROP_APP_ID,
  PROP_BUS_NAME,
  PROP_BUS_PATH,
  PROP_AUTOSTART,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };

static void
got_proxy (GObject      *source_object,
           GAsyncResult *res,
           gpointer      user_data)
{
  PhoshSearchProvider *self = PHOSH_SEARCH_PROVIDER (user_data);
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);
  g_autoptr (GError) error = NULL;

  priv->proxy = phosh_dbus_search_provider2_proxy_new_for_bus_finish (res, &error);

  if (error) {
    g_warning ("Unable to create proxy for %s-%s: %s",
               priv->bus_name,
               priv->bus_path,
               error->message);
    return;
  }

  g_debug ("Got proxy for %s%s",
           priv->bus_name,
           priv->bus_path);
}

static void
phosh_search_provider_constructed (GObject *object)
{
  PhoshSearchProvider *self = PHOSH_SEARCH_PROVIDER (object);
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  G_OBJECT_CLASS (phosh_search_provider_parent_class)->constructed (object);

  phosh_dbus_search_provider2_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                                 priv->proxy_flags,
                                                 priv->bus_name,
                                                 priv->bus_path,
                                                 priv->cancellable,
                                                 got_proxy,
                                                 self);
}

static void
phosh_search_provider_finalize (GObject *object)
{
  PhoshSearchProvider *self = PHOSH_SEARCH_PROVIDER (object);
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  // Stop any operations still in flight
  g_cancellable_cancel (priv->cancellable);

  g_clear_object (&priv->info);
  g_clear_object (&priv->proxy);
  g_clear_object (&priv->cancellable);

  g_clear_pointer (&priv->bus_name, g_free);
  g_clear_pointer (&priv->bus_path, g_free);

  G_OBJECT_CLASS (phosh_search_provider_parent_class)->finalize (object);
}

static void
phosh_search_provider_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PhoshSearchProvider *self = PHOSH_SEARCH_PROVIDER (object);
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  switch (property_id) {
    case PROP_APP_INFO:
      priv->info = g_value_dup_object (value);
      break;
    case PROP_BUS_NAME:
      priv->bus_name = g_value_dup_string (value);
      break;
    case PROP_BUS_PATH:
      priv->bus_path = g_value_dup_string (value);
      break;
    case PROP_AUTOSTART:
      priv->autostart = g_value_get_boolean (value);
      if (priv->autostart) {
        // Delay autostart to avoid spawing everything at once
        priv->proxy_flags |= G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION;
      } else {
        // Don't attempt to autostart
        priv->proxy_flags |= G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
phosh_search_provider_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PhoshSearchProvider *self = PHOSH_SEARCH_PROVIDER (object);
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  switch (property_id) {
    case PROP_APP_INFO:
      g_value_set_object (value, priv->info);
      break;
    case PROP_APP_ID:
      g_value_set_string (value, g_app_info_get_id (priv->info));
      break;
    case PROP_BUS_NAME:
      g_value_set_string (value, priv->bus_name);
      break;
    case PROP_BUS_PATH:
      g_value_set_string (value, priv->bus_path);
      break;
    case PROP_AUTOSTART:
      g_value_set_boolean (value, priv->autostart);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
phosh_search_provider_class_init (PhoshSearchProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_search_provider_constructed;
  object_class->finalize = phosh_search_provider_finalize;
  object_class->set_property = phosh_search_provider_set_property;
  object_class->get_property = phosh_search_provider_get_property;

  pspecs[PROP_APP_INFO] =
    g_param_spec_object ("app-info", "App info", "Application info",
                         G_TYPE_APP_INFO,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  pspecs[PROP_APP_ID] =
    g_param_spec_string ("app-id", "App id", "Application id",
                         NULL,
                         G_PARAM_READABLE);

  pspecs[PROP_BUS_NAME] =
    g_param_spec_string ("bus-name", "Bus name", "D-Bus name",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  pspecs[PROP_BUS_PATH] =
    g_param_spec_string ("bus-path", "Bus path", "D-Bus object path",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  pspecs[PROP_AUTOSTART] =
    g_param_spec_boolean ("autostart", "Autostart", "Autostart the service",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}

static void
phosh_search_provider_init (PhoshSearchProvider *self)
{
  PhoshSearchProviderPrivate *priv = phosh_search_provider_get_instance_private (self);

  priv->bus_name = NULL;
  priv->bus_path = NULL;
  priv->cancellable = NULL;
  priv->info = NULL;
  priv->proxy = NULL;
  priv->proxy_flags = G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES;
  priv->autostart = TRUE;
}

PhoshSearchProvider *
phosh_search_provider_new (const char *desktop_app_id,
                           const char *bus_path,
                           const char *bus_name,
                           gboolean    autostart)
{
  return g_object_new (PHOSH_TYPE_SEARCH_PROVIDER,
                       "app-info", g_desktop_app_info_new (desktop_app_id),
                       "bus-path", bus_path,
                       "bus-name", bus_name,
                       "autostart", autostart,
                       NULL);
}
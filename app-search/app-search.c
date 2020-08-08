/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-app-search"

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include "gnome-shell-search-provider.h"


#define PHOSH_TYPE_APP_SEARCH_APPLICATION (phosh_app_search_application_get_type ())

G_DECLARE_FINAL_TYPE (PhoshAppSearchApplication, phosh_app_search_application, PHOSH, APP_SEARCH_APPLICATION, GApplication)


struct _PhoshAppSearchApplication {
  GApplication              parent;

  PhoshDBusSearchProvider2 *provider;
};


G_DEFINE_TYPE (PhoshAppSearchApplication, phosh_app_search_application, G_TYPE_APPLICATION)


static void
phosh_app_search_application_dispose (GObject *object)
{
  PhoshAppSearchApplication *self = PHOSH_APP_SEARCH_APPLICATION (object);

  g_clear_object (&self->provider);

  G_OBJECT_CLASS (phosh_app_search_application_parent_class)->dispose (object);
}


static void
phosh_app_search_application_activate (GApplication *application)
{
  /* GApplication complains if we don't provide an activate */
}


static gboolean
phosh_app_search_application_dbus_register (GApplication     *application,
                                            GDBusConnection  *connection,
                                            const gchar      *object_path,
                                            GError          **error)
{
  PhoshAppSearchApplication *self = PHOSH_APP_SEARCH_APPLICATION (application);

  if (!G_APPLICATION_CLASS (phosh_app_search_application_parent_class)->dbus_register (application,
                                                                                       connection,
                                                                                       object_path,
                                                                                       error)) {
    return FALSE;
  }

  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self->provider),
                                         connection,
                                         object_path,
                                         error)) {
    g_critical ("Can’t export search provider: %s", (*error)->message);
  }

  return TRUE;
}


static void
phosh_app_search_application_dbus_unregister (GApplication     *application,
                                              GDBusConnection  *connection,
                                              const gchar      *object_path)
{
  PhoshAppSearchApplication *self = PHOSH_APP_SEARCH_APPLICATION (application);

  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (self->provider));

  G_APPLICATION_CLASS (phosh_app_search_application_parent_class)->dbus_unregister (application,
                                                                                    connection,
                                                                                    object_path);
}


static void
phosh_app_search_application_class_init (PhoshAppSearchApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->dispose = phosh_app_search_application_dispose;

  app_class->activate = phosh_app_search_application_activate;
  app_class->dbus_register = phosh_app_search_application_dbus_register;
  app_class->dbus_unregister = phosh_app_search_application_dbus_unregister;
}


static GStrv
do_search (GStrv terms)
{
  GPtrArray *strv_builder = g_ptr_array_new_full (10, g_free);
  g_autofree char *search_string = NULL;
  g_autofree char ***search_results = NULL;

  search_string = g_strjoinv (" ", (GStrv) terms);
  search_results = g_desktop_app_info_search (search_string);

  for (int i = 0; search_results[i]; i++) {
    for (int j = 0; search_results[i][j]; j++) {
      g_autoptr (GDesktopAppInfo) info = NULL;

      info = g_desktop_app_info_new (search_results[i][j]);

      if (G_UNLIKELY (info == NULL)) {
        g_warning ("Unknown desktop-id: “%s”", search_results[i][j]);

        continue;
      }

      if (G_LIKELY (g_app_info_should_show (G_APP_INFO (info)))) {
        g_ptr_array_add (strv_builder,
                         g_strdup (g_app_info_get_id (G_APP_INFO (info))));
      }
    }
  }

  g_ptr_array_add (strv_builder, NULL);

  for (int i = 0; search_results[i]; i++) {
    g_strfreev (search_results[i]);
  }

  return (GStrv) g_ptr_array_free (strv_builder, FALSE);
}


static gboolean
get_initial_result_set (PhoshDBusSearchProvider2  *interface,
                        GDBusMethodInvocation     *invocation,
                        const char *const         *terms,
                        PhoshAppSearchApplication *self)
{
  g_auto (GStrv) results = NULL;

  g_application_hold (G_APPLICATION (self));

  results = do_search ((GStrv) terms);

  phosh_dbus_search_provider2_complete_get_initial_result_set (interface,
                                                               invocation,
                                                               (const char *const *) results);

  g_application_release (G_APPLICATION (self));

  return TRUE;
}


static gboolean
get_subsearch_result_set (PhoshDBusSearchProvider2  *interface,
                          GDBusMethodInvocation     *invocation,
                          const char *const         *previous_results,
                          const char *const         *terms,
                          PhoshAppSearchApplication *self)
{
  g_auto (GStrv) results = NULL;

  g_application_hold (G_APPLICATION (self));

  results = do_search ((GStrv) terms);

  phosh_dbus_search_provider2_complete_get_subsearch_result_set (interface,
                                                                 invocation,
                                                                 (const char *const *) results);

  g_application_release (G_APPLICATION (self));

  return TRUE;
}


static gboolean
get_result_metas (PhoshDBusSearchProvider2  *interface,
                  GDBusMethodInvocation     *invocation,
                  const char *const         *results,
                  PhoshAppSearchApplication *self)
{
  GVariantBuilder builder;

  g_application_hold (G_APPLICATION (self));

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("aa{sv}"));

  for (int i = 0; results[i]; i++) {
    g_autoptr (GDesktopAppInfo) info = NULL;
    g_autofree char *title = NULL;
    g_autofree char *description = NULL;
    GIcon *icon = NULL;

    info = g_desktop_app_info_new (results[i]);

    if (G_UNLIKELY (info == NULL)) {
      g_warning ("Unknown desktop-id: “%s”", results[i]);

      continue;
    }

    g_variant_builder_open (&builder, G_VARIANT_TYPE ("a{sv}"));

    g_variant_builder_add (&builder,
                           "{sv}",
                           "id",
                           g_variant_new_string (results[i]));

    title = g_markup_escape_text (g_app_info_get_name (G_APP_INFO (info)), -1);
    g_variant_builder_add (&builder,
                           "{sv}",
                           "name",
                           g_variant_new_string (title));

    description = g_markup_escape_text (g_app_info_get_description (G_APP_INFO (info)), -1);
    g_variant_builder_add (&builder,
                           "{sv}",
                           "description",
                           g_variant_new_string (description));

    icon = g_app_info_get_icon (G_APP_INFO (info));
    g_variant_builder_add (&builder, "{sv}", "icon", g_icon_serialize (icon));

    g_variant_builder_close (&builder);
  }

  phosh_dbus_search_provider2_complete_get_result_metas (interface,
                                                         invocation,
                                                         g_variant_builder_end (&builder));

  g_application_release (G_APPLICATION (self));

  return TRUE;
}


static gboolean
activate_result (PhoshDBusSearchProvider2  *interface,
                 GDBusMethodInvocation     *invocation,
                 const char                *id,
                 const char *const         *terms,
                 guint                      timestamp,
                 PhoshAppSearchApplication *self)
{
  g_autoptr (GDesktopAppInfo) info = NULL;
  g_autoptr (GError) error = NULL;

  g_application_hold (G_APPLICATION (self));

  info = g_desktop_app_info_new (id);

  if (!info) {
    g_warning ("Unknown desktop-id: “%s”", id);

    g_dbus_method_invocation_return_error (invocation,
                                           G_DBUS_ERROR,
                                           G_DBUS_ERROR_FAILED,
                                           "Unknown desktop-id: “%s”",
                                           id);

    g_application_release (G_APPLICATION (self));

    return TRUE;
  }

  g_app_info_launch (G_APP_INFO (info), NULL, NULL, &error);

  if (error) {
    g_warning ("Failed to launch app “%s”: %s",
               id,
               error->message);

    g_dbus_method_invocation_return_error (invocation,
                                           G_DBUS_ERROR,
                                           G_DBUS_ERROR_FAILED,
                                           "Failed to launch app “%s”: %s",
                                           id,
                                           error->message);

    g_application_release (G_APPLICATION (self));

    return TRUE;
  }

  phosh_dbus_search_provider2_complete_activate_result (interface, invocation);

  g_application_release (G_APPLICATION (self));

  return TRUE;
}


static gboolean
launch_search (PhoshDBusSearchProvider2  *interface,
               GDBusMethodInvocation     *invocation,
               const char *const         *terms,
               guint                      timestamp,
               PhoshAppSearchApplication *self)
{
  g_application_hold (G_APPLICATION (self));

  phosh_dbus_search_provider2_complete_launch_search (interface, invocation);

  g_application_release (G_APPLICATION (self));

  return TRUE;
}


static void
phosh_app_search_application_init (PhoshAppSearchApplication *self)
{
  self->provider = phosh_dbus_search_provider2_skeleton_new ();

  g_signal_connect (self->provider,
                    "handle-get-initial-result-set",
                    G_CALLBACK (get_initial_result_set),
                    self);
  g_signal_connect (self->provider,
                    "handle-get-subsearch-result-set",
                    G_CALLBACK (get_subsearch_result_set),
                    self);
  g_signal_connect (self->provider,
                    "handle-get-result-metas",
                    G_CALLBACK (get_result_metas),
                    self);
  g_signal_connect (self->provider,
                    "handle-activate-result",
                    G_CALLBACK (activate_result),
                    self);
  g_signal_connect (self->provider,
                    "handle-launch-search",
                    G_CALLBACK (launch_search),
                    self);
}


#ifdef TESTING_APP_SEARCH
static int
real_main (int argc, char **argv)
#else
int
main (int argc, char **argv)
#endif
{
  g_autoptr (GApplication) app = NULL;

  app = g_object_new (PHOSH_TYPE_APP_SEARCH_APPLICATION,
                      "application-id", "sm.puri.Phosh.AppSearch",
                      "flags", G_APPLICATION_IS_SERVICE,
                      NULL);

  return g_application_run (app, argc, argv);
}

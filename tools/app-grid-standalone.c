/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * BUILDDIR $ ./run_tool ./tools/app-grid-standalone
 *
 * PhoshAppGrid in a simple wrapper
 */

#include <app-grid.h>

#include "search/search.h"
#include "search/search-provider.h"

static void
css_setup (void)
{
  GtkCssProvider *provider;
  GFile *file;
  GError *error = NULL;

  provider = gtk_css_provider_new ();
  file = g_file_new_for_uri ("resource:///sm/puri/phosh/style.css");

  if (!gtk_css_provider_load_from_file (provider, file, &error)) {
    g_warning ("Failed to load CSS file: %s", error->message);
    g_clear_error (&error);
    g_object_unref (file);
    return;
  }
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (provider), 600);
  g_object_unref (file);
}

static void
got_metas (GObject      *source,
           GAsyncResult *res,
           gpointer      data)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GPtrArray) metas = NULL;

  metas = phosh_search_provider_get_result_meta_finish (PHOSH_SEARCH_PROVIDER (source), res, &error);

  for (int i = 0; i < metas->len; i++) {
    PhoshSearchProviderResultMeta *result = g_ptr_array_index (metas, i);

    g_message ("%s: %s",
               phosh_search_provider_result_meta_get_id (result),
               phosh_search_provider_result_meta_get_name (result));
  }

  if (error) {
    g_warning ("%s", error->message);
  }
}

static void
got_initial (GObject      *source,
             GAsyncResult *res,
             gpointer      data)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GPtrArray) filtered = NULL;
  GStrv results = NULL;
  GStrv results_filtered = NULL;

  results = phosh_search_provider_get_initial_finish (PHOSH_SEARCH_PROVIDER (source), res, &error);

  if (error) {
    g_warning ("Failed to search provider: %s", error->message);
    return;
  }

  filtered = phosh_search_provider_limit_results (results, 5);

  if (!filtered) {
    g_debug ("No results");
    return;
  }

  results_filtered = g_new (char *, filtered->len + 1);

  for (int i = 0; i < filtered->len; i++) {
    results_filtered[i] = g_ptr_array_index (filtered, i);
  }
  results_filtered[filtered->len] = NULL;

  phosh_search_provider_get_result_meta (PHOSH_SEARCH_PROVIDER (source), results_filtered, got_metas, NULL);
}

static void
ready (PhoshSearchProvider *self)
{
  const char *terms[] = {
    "test",
    NULL
  };

  phosh_search_provider_get_initial (self, terms, got_initial, NULL);
}

int
main (int argc, char *argv[])
{
  GtkWidget *win;
  GtkWidget *widget;
  g_autoptr (PhoshSearchProvider) provider = NULL;

  gtk_init (&argc, &argv);

  css_setup ();

  g_object_set (gtk_settings_get_default (),
                "gtk-application-prefer-dark-theme", TRUE,
                NULL);

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (win, "delete_event", G_CALLBACK (gtk_main_quit), NULL);

  gtk_widget_show (win);

  widget = g_object_new (PHOSH_TYPE_APP_GRID, NULL);

  gtk_widget_show (widget);

  gtk_container_add (GTK_CONTAINER (win), widget);

  g_object_new (PHOSH_TYPE_SEARCH, NULL);

  provider = phosh_search_provider_new ("org.gnome.Nautilus.desktop",
                                        "/org/gnome/Nautilus/SearchProvider",
                                        "org.gnome.Nautilus",
                                        TRUE,
                                        FALSE);
  g_signal_connect (provider, "ready", G_CALLBACK (ready), NULL);

  gtk_main ();

  return 0;
}

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

static gboolean
ready (gpointer data)
{
  PhoshSearch *search = data;
  char *terms[] = {"test", "me", NULL };

  phosh_search_set_terms (search, terms);

  return G_SOURCE_REMOVE;
}

int
main (int argc, char *argv[])
{
  GtkWidget *win;
  GtkWidget *widget;
  g_autoptr (PhoshSearch) search = NULL;

  gtk_init (&argc, &argv);

  css_setup ();

  g_object_set (gtk_settings_get_default (),
                "gtk-application-prefer-dark-theme", TRUE,
                NULL);

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (win, "delete_event", G_CALLBACK (gtk_main_quit), NULL);
  gtk_window_set_default_size (GTK_WINDOW (win), 360, 720);

  gtk_widget_show (win);

  widget = g_object_new (PHOSH_TYPE_APP_GRID, NULL);

  gtk_widget_show (widget);

  gtk_container_add (GTK_CONTAINER (win), widget);

  search = g_object_new (PHOSH_TYPE_SEARCH, NULL);

  g_timeout_add (2500, ready, search);

  gtk_main ();

  return 0;
}

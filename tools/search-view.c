/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include <search-result-list.h>
#include <search-result-frame.h>
#include <search-result-app-frame.h>


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


static GtkWidget *
create_frame (gpointer item, gpointer data)
{
  PhoshSearchResultSource *source = item;
  const char *source_id = NULL;
  GtkWidget *frame;

  source_id = phosh_search_result_source_get_id (source);

  if (G_UNLIKELY (g_strcmp0 (source_id, "/search/app") == 0)) {
    frame = phosh_search_result_app_frame_new ();
    phosh_search_result_app_frame_bind_source (PHOSH_SEARCH_RESULT_APP_FRAME (frame),
                                               source);

    return frame;
  }

  frame = phosh_search_result_frame_new ();
  phosh_search_result_frame_bind_source (PHOSH_SEARCH_RESULT_FRAME (frame),
                                         source);


  return frame;
}


int
main (int argc, char **argv)
{
  g_autoptr (PhoshSearchResultList) list = NULL;
  g_autoptr (PhoshSearchSource) app_source_meta = NULL;
  g_autoptr (PhoshSearchResultSource) app_source = NULL;
  g_autoptr (PhoshSearchSource) source_meta = NULL;
  g_autoptr (PhoshSearchResultSource) source = NULL;
  g_autoptr (PhoshSearchResultMeta) app_res_1_meta = NULL;
  g_autoptr (PhoshSearchResult) app_res_1 = NULL;
  g_autoptr (PhoshSearchResultMeta) app_res_2_meta = NULL;
  g_autoptr (PhoshSearchResult) app_res_2 = NULL;
  g_autoptr (PhoshSearchResultMeta) res_meta = NULL;
  g_autoptr (PhoshSearchResult) res = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (GAppInfo) info = NULL;
  GtkWidget *win;
  GtkWidget *listbox;

  gtk_init (&argc, &argv);

  css_setup ();

  g_object_set (gtk_settings_get_default (),
                "gtk-application-prefer-dark-theme", TRUE,
                NULL);

  icon = g_themed_icon_new ("help-about");
  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));

  list = phosh_search_result_list_new ();

  app_source_meta = phosh_search_source_new ("/search/app", info);
  app_source = phosh_search_result_source_new (app_source_meta);
  phosh_search_result_list_add (list, app_source);

  app_res_1_meta = phosh_search_result_meta_new ("demo.app.First.desktop",
                                                 "First",
                                                 NULL,
                                                 NULL,
                                                 NULL);
  app_res_1 = phosh_search_result_new (app_res_1_meta);
  phosh_search_result_source_add (app_source, app_res_1);

  app_res_2_meta = phosh_search_result_meta_new ("demo.app.Second.desktop",
                                                 "Second",
                                                 NULL,
                                                 NULL,
                                                 NULL);
  app_res_2 = phosh_search_result_new (app_res_2_meta);
  phosh_search_result_source_add (app_source, app_res_2);


  source_meta = phosh_search_source_new ("/search/something", info);
  source = phosh_search_result_source_new (source_meta);
  phosh_search_result_list_add (list, source);

  res_meta = phosh_search_result_meta_new ("demo.app.First.desktop",
                                           "Some result",
                                           "for the current search",
                                           icon,
                                           NULL);
  res = phosh_search_result_new (res_meta);
  phosh_search_result_source_add (source, res);


  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (win, "delete_event", G_CALLBACK (gtk_main_quit), NULL);
  gtk_window_set_default_size (GTK_WINDOW (win), 360, 720);

  listbox = gtk_list_box_new ();
  gtk_widget_show (listbox);
  gtk_container_add (GTK_CONTAINER (win), listbox);

  gtk_list_box_bind_model (GTK_LIST_BOX (listbox),
                           G_LIST_MODEL (list),
                           create_frame,
                           NULL,
                           NULL);

  gtk_widget_show (win);

  gtk_main ();

  return 0;
}

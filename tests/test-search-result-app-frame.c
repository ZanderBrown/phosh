/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "search/search-result-app-frame.c"
#include "stubs/bad-instance.h"
#include "stubs/bad-prop.h"


static void
test_phosh_search_result_app_frame_new (void)
{
  g_autoptr (PhoshSearchResult) result = NULL;
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (PhoshSearchResultMeta) meta_dest = NULL;
  g_autoptr (PhoshSearchResultSource) model = NULL;
  g_autoptr (PhoshSearchSource) source = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (GAppInfo) info = NULL;
  GtkWidget *frame = NULL;

  icon = g_themed_icon_new ("start-here");

  meta = phosh_search_result_meta_new ("demo.app.Second.desktop",
                                       "Test",
                                       "Result",
                                       icon,
                                       "copy-me");
  g_assert_nonnull (meta);

  result = phosh_search_result_new (meta);
  g_assert_nonnull (result);

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));
  g_assert_nonnull (info);

  source = phosh_search_source_new ("test", info);
  g_assert_nonnull (source);

  model = phosh_search_result_source_new (source);
  g_assert_nonnull (model);

  phosh_search_result_source_add (model, result);

  frame = phosh_search_result_app_frame_new ();
  g_assert_nonnull (frame);

  gtk_widget_destroy (frame);
}


static void
test_phosh_search_result_app_frame_bind (void)
{
  g_autoptr (PhoshSearchResult) result = NULL;
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (PhoshSearchResultMeta) meta_dest = NULL;
  g_autoptr (PhoshSearchResultSource) model = NULL;
  g_autoptr (PhoshSearchSource) source = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (GAppInfo) info = NULL;
  GtkWidget *frame = NULL;

  icon = g_themed_icon_new ("start-here");

  meta = phosh_search_result_meta_new ("demo.app.Second.desktop",
                                       "Test",
                                       "Result",
                                       icon,
                                       "copy-me");
  g_assert_nonnull (meta);

  result = phosh_search_result_new (meta);
  g_assert_nonnull (result);

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));
  g_assert_nonnull (info);

  source = phosh_search_source_new ("test", info);
  g_assert_nonnull (source);

  model = phosh_search_result_source_new (source);
  g_assert_nonnull (model);

  phosh_search_result_source_add (model, result);

  frame = phosh_search_result_app_frame_new ();
  g_assert_nonnull (frame);

  phosh_search_result_app_frame_bind_source (PHOSH_SEARCH_RESULT_APP_FRAME (frame),
                                             model);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 1);

  g_clear_object (&model);

  /* Rebind to a new model */
  model = phosh_search_result_source_new (source);
  g_assert_nonnull (model);

  phosh_search_result_app_frame_bind_source (PHOSH_SEARCH_RESULT_APP_FRAME (frame),
                                             model);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 0);

  /* Rebind to invalid model */
  NULL_INSTANCE_CALL (phosh_search_result_app_frame_bind_source,
                      "PHOSH_IS_SEARCH_RESULT_APP_FRAME (self)",
                      NULL);
  g_test_expect_message (G_LOG_DOMAIN,
                         G_LOG_LEVEL_CRITICAL,
                         "phosh_search_result_app_frame_bind_source: assertion 'PHOSH_IS_SEARCH_RESULT_SOURCE (source)' failed");
  phosh_search_result_app_frame_bind_source (PHOSH_SEARCH_RESULT_APP_FRAME (frame), NULL);


  gtk_widget_destroy (frame);
}


static void
test_phosh_search_result_app_frame_activate (void)
{
  g_autoptr (PhoshSearchResult) result = NULL;
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (PhoshSearchResultMeta) meta_dest = NULL;
  g_autoptr (PhoshSearchResultSource) model = NULL;
  g_autoptr (PhoshSearchSource) source = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (GAppInfo) info = NULL;
  GtkWidget *frame = NULL;
  GtkFlowBoxChild *child = NULL;

  icon = g_themed_icon_new ("start-here");

  meta = phosh_search_result_meta_new ("demo.app.Second.desktop",
                                       "Test",
                                       "Result",
                                       icon,
                                       "copy-me");
  g_assert_nonnull (meta);

  result = phosh_search_result_new (meta);
  g_assert_nonnull (result);

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));
  g_assert_nonnull (info);

  source = phosh_search_source_new ("test", info);
  g_assert_nonnull (source);

  model = phosh_search_result_source_new (source);
  g_assert_nonnull (model);

  phosh_search_result_source_add (model, result);

  frame = phosh_search_result_app_frame_new ();
  g_assert_nonnull (frame);

  phosh_search_result_app_frame_bind_source (PHOSH_SEARCH_RESULT_APP_FRAME (frame),
                                             model);

  child = gtk_flow_box_get_child_at_index (GTK_FLOW_BOX (PHOSH_SEARCH_RESULT_APP_FRAME (frame)->apps), 0);

  item_activated (GTK_FLOW_BOX (PHOSH_SEARCH_RESULT_APP_FRAME (frame)->apps),
                  child,
                  PHOSH_SEARCH_RESULT_APP_FRAME (frame));

  gtk_widget_destroy (frame);
}


int
main (int argc, char **argv)
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/search-result-app-frame/new",
                   test_phosh_search_result_app_frame_new);
  g_test_add_func ("/phosh/search-result-app-frame/bind",
                   test_phosh_search_result_app_frame_bind);
  g_test_add_func ("/phosh/search-result-appframe/activate",
                   test_phosh_search_result_app_frame_activate);

  return g_test_run ();
}

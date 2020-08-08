/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "search/search-result-source.c"
#include "stubs/bad-instance.h"
#include "stubs/bad-prop.h"


static void
test_phosh_search_result_source_new (void)
{
  g_autoptr (PhoshSearchResultSource) model = NULL;
  g_autoptr (PhoshSearchSource) source = NULL;
  g_autoptr (PhoshSearchSource) source_dest = NULL;
  g_autoptr (GAppInfo) info = NULL;

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));
  g_assert_nonnull (info);

  source = phosh_search_source_new ("test", info);
  g_assert_nonnull (source);

  model = phosh_search_result_source_new (source);
  g_assert_nonnull (model);

  g_object_get (model, "source", &source_dest, NULL);

  g_assert_true (source == source_dest);
}


static void
test_phosh_search_result_source_get (void)
{
  g_autoptr (PhoshSearchResultSource) model = NULL;
  g_autoptr (PhoshSearchSource) source = NULL;
  g_autoptr (PhoshSearchSource) source_dest = NULL;
  g_autoptr (GAppInfo) info = NULL;
  g_autoptr (GAppInfo) info_dest = NULL;
  g_autofree char *id = NULL;
  guint total = 12;

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));
  g_assert_nonnull (info);

  source = phosh_search_source_new ("test", info);
  g_assert_nonnull (source);

  model = phosh_search_result_source_new (source);
  g_assert_nonnull (model);

  g_object_get (model,
                "id", &id,
                "app-info", &info_dest,
                "total-results", &total,
                NULL);

  g_assert_cmpstr (id, ==, phosh_search_source_get_id (source));
  g_assert_true (g_app_info_equal (info, info_dest));
  g_assert_cmpuint (total, ==, 0);

  BAD_PROP (model, phosh_search_result_source, PhoshSearchResultSource);
}


static void
test_phosh_search_result_source_get_accessors (void)
{
  g_autoptr (PhoshSearchResultSource) model = NULL;
  g_autoptr (PhoshSearchSource) source = NULL;
  g_autoptr (GAppInfo) info = NULL;

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));
  g_assert_nonnull (info);

  source = phosh_search_source_new ("test", info);
  g_assert_nonnull (source);

  model = phosh_search_result_source_new (source);
  g_assert_nonnull (model);

  g_assert_true (phosh_search_result_source_get_source (model) == source);
  g_assert_cmpstr (phosh_search_result_source_get_id (model),
                   ==,
                   phosh_search_source_get_id (source));
  g_assert_true (phosh_search_result_source_get_app_info (model) == info);
  g_assert_cmpuint (phosh_search_result_source_get_total_results (model),
                    ==,
                    0);
}


static void
test_phosh_search_result_source_null_instance (void)
{
  g_autoptr (PhoshSearchResultSource) source = NULL;

  NULL_INSTANCE_CALL (phosh_search_result_source_add,
                      "PHOSH_IS_SEARCH_RESULT_SOURCE (self)",
                      NULL);
  NULL_INSTANCE_CALL_RETURN (phosh_search_result_source_get_source,
                             "PHOSH_IS_SEARCH_RESULT_SOURCE (self)",
                             NULL);
  NULL_INSTANCE_CALL_RETURN (phosh_search_result_source_get_id,
                             "PHOSH_IS_SEARCH_RESULT_SOURCE (self)",
                             NULL);
  NULL_INSTANCE_CALL_RETURN (phosh_search_result_source_get_app_info,
                             "PHOSH_IS_SEARCH_RESULT_SOURCE (self)",
                             NULL);
  NULL_INSTANCE_CALL_RETURN (phosh_search_result_source_get_total_results,
                             "PHOSH_IS_SEARCH_RESULT_SOURCE (self)",
                             0);
}


static void
test_phosh_search_result_source_g_list_iface (void)
{
  g_autoptr (PhoshSearchResultSource) model = NULL;
  g_autoptr (PhoshSearchSource) source = NULL;
  g_autoptr (GAppInfo) info = NULL;

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));
  g_assert_nonnull (info);

  source = phosh_search_source_new ("test", info);
  g_assert_nonnull (source);

  model = phosh_search_result_source_new (source);
  g_assert_nonnull (model);

  g_assert_true (G_IS_LIST_MODEL (model));

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 0);
  g_assert_true (g_list_model_get_item_type (G_LIST_MODEL (model)) == PHOSH_TYPE_SEARCH_RESULT);
}


static gboolean emitted = FALSE;


static void
changed (GListModel *list,
         guint       position,
         guint       removed,
         guint       added,
         gpointer    user_data)
{
  emitted = TRUE;

  g_assert_cmpuint (position, ==, 0);
  g_assert_cmpuint (removed, ==, 0);
  g_assert_cmpuint (added, ==, 1);
}


static void
test_phosh_search_result_source_add (void)
{
  g_autoptr (PhoshSearchResultSource) model = NULL;
  g_autoptr (PhoshSearchSource) source = NULL;
  g_autoptr (PhoshSearchResult) result = NULL;
  g_autoptr (PhoshSearchResult) result_dest = NULL;
  g_autoptr (GAppInfo) info = NULL;

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));
  g_assert_nonnull (info);

  source = phosh_search_source_new ("test", info);
  g_assert_nonnull (source);

  model = phosh_search_result_source_new (source);
  g_assert_nonnull (model);

  g_assert_false (emitted);
  g_signal_connect (model, "items-changed", G_CALLBACK (changed), NULL);

  result = phosh_search_result_new (NULL);

  phosh_search_result_source_add (model, result);

  g_assert_true (emitted);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (model)), ==, 1);

  result_dest = g_list_model_get_item (G_LIST_MODEL (model), 0);

  g_assert_true (result == result_dest);
}


int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/search-result-source/new",
                   test_phosh_search_result_source_new);
  g_test_add_func ("/phosh/search-result-source/get",
                   test_phosh_search_result_source_get);
  g_test_add_func ("/phosh/search-result-source/get/accessors",
                   test_phosh_search_result_source_get_accessors);
  g_test_add_func ("/phosh/search-result-source/null-instance",
                   test_phosh_search_result_source_null_instance);
  g_test_add_func ("/phosh/search-result-source/g_list_iface",
                   test_phosh_search_result_source_g_list_iface);
  g_test_add_func ("/phosh/search-result-source/add",
                   test_phosh_search_result_source_add);

  return g_test_run ();
}

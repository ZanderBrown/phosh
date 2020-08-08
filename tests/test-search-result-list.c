/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "search/search-result-list.c"
#include "stubs/bad-instance.h"
#include "stubs/bad-prop.h"


static void
test_phosh_search_result_list_new (void)
{
  g_autoptr (PhoshSearchResultList) list = NULL;

  list = phosh_search_result_list_new ();

  g_assert_nonnull (list);
  g_assert_true (PHOSH_IS_SEARCH_RESULT_LIST (list));
}


static void
test_phosh_search_result_list_null_instance (void)
{
  g_autoptr (PhoshSearchResultList) list = NULL;

  NULL_INSTANCE_CALL (phosh_search_result_list_add,
                      "PHOSH_IS_SEARCH_RESULT_LIST (self)",
                      NULL);
}


static void
test_phosh_search_result_list_g_list_iface (void)
{
  g_autoptr (PhoshSearchResultList) list = NULL;

  list = phosh_search_result_list_new ();

  g_assert_true (G_IS_LIST_MODEL (list));

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (list)), ==, 0);
  g_assert_true (g_list_model_get_item_type (G_LIST_MODEL (list)) == PHOSH_TYPE_SEARCH_RESULT_SOURCE);
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
test_phosh_search_result_list_add (void)
{
  g_autoptr (PhoshSearchResultList) list = NULL;
  g_autoptr (PhoshSearchResultSource) model = NULL;
  g_autoptr (PhoshSearchResultSource) model_dest = NULL;
  g_autoptr (PhoshSearchSource) source = NULL;
  g_autoptr (PhoshSearchResult) result = NULL;
  g_autoptr (GAppInfo) info = NULL;

  list = phosh_search_result_list_new ();

  g_assert_false (emitted);
  g_signal_connect (list, "items-changed", G_CALLBACK (changed), NULL);

  info = G_APP_INFO (g_desktop_app_info_new ("demo.app.Second.desktop"));
  g_assert_nonnull (info);

  source = phosh_search_source_new ("test", info);
  g_assert_nonnull (source);

  model = phosh_search_result_source_new (source);
  g_assert_nonnull (model);

  phosh_search_result_list_add (list, model);

  g_assert_true (emitted);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (list)), ==, 1);

  model_dest = g_list_model_get_item (G_LIST_MODEL (list), 0);

  g_assert_true (model == model_dest);
}


int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/search-result-source/new",
                   test_phosh_search_result_list_new);
  g_test_add_func ("/phosh/search-result-source/null-instance",
                   test_phosh_search_result_list_null_instance);
  g_test_add_func ("/phosh/search-result-source/g_list_iface",
                   test_phosh_search_result_list_g_list_iface);
  g_test_add_func ("/phosh/search-result-source/add",
                   test_phosh_search_result_list_add);

  return g_test_run ();
}

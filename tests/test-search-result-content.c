/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "search/search-result-content.c"
#include "stubs/bad-instance.h"
#include "stubs/bad-prop.h"


static void
test_phosh_search_result_content_new (void)
{
  g_autoptr (PhoshSearchResult) result = NULL;
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (PhoshSearchResultMeta) meta_dest = NULL;
  g_autoptr (GIcon) icon = NULL;
  GtkWidget *content = NULL;
  PhoshSearchResult *result_dest = NULL;

  icon = g_themed_icon_new ("start-here");

  meta = phosh_search_result_meta_new ("test",
                                       "Test",
                                       "Result",
                                       icon,
                                       "copy-me");
  g_assert_nonnull (meta);

  result = phosh_search_result_new (meta);
  g_assert_nonnull (result);

  content = phosh_search_result_content_new (result);

  result_dest = phosh_search_result_content_get_result (PHOSH_SEARCH_RESULT_CONTENT (content));
  g_assert_true (result == result_dest);

  NULL_INSTANCE_CALL_RETURN (phosh_search_result_content_get_result,
                             "PHOSH_IS_SEARCH_RESULT_CONTENT (self)",
                             NULL);
  NULL_INSTANCE_CALL (phosh_search_result_content_set_result,
                      "PHOSH_IS_SEARCH_RESULT_CONTENT (self)",
                      NULL);
  g_test_expect_message (G_LOG_DOMAIN,
                         G_LOG_LEVEL_CRITICAL,
                         "phosh_search_result_content_set_result: assertion 'PHOSH_IS_SEARCH_RESULT (result)' failed");
  phosh_search_result_content_set_result (PHOSH_SEARCH_RESULT_CONTENT (content), NULL);

  gtk_widget_destroy (content);
}


static void
test_phosh_search_result_content_get (void)
{
  g_autoptr (PhoshSearchResult) result = NULL;
  g_autoptr (PhoshSearchResult) result_dest = NULL;
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (PhoshSearchResultMeta) meta_dest = NULL;
  g_autoptr (GIcon) icon = NULL;
  GtkWidget *content = NULL;

  icon = g_themed_icon_new ("start-here");

  meta = phosh_search_result_meta_new ("test",
                                       "Test",
                                       "Result",
                                       icon,
                                       "copy-me");
  g_assert_nonnull (meta);

  result = phosh_search_result_new (meta);
  g_assert_nonnull (result);

  content = phosh_search_result_content_new (result);

  g_object_get (content, "result", &result_dest, NULL);
  g_assert_true (result == result_dest);

  BAD_PROP (content, phosh_search_result_content, PhoshSearchResultContent);

  gtk_widget_destroy (content);
}


int
main (int argc, char **argv)
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/search-result-content/new",
                   test_phosh_search_result_content_new);
  g_test_add_func ("/phosh/search-result-content/get",
                   test_phosh_search_result_content_get);

  return g_test_run ();
}

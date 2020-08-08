/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "search/search-result.c"
#include "stubs/bad-instance.h"
#include "stubs/bad-prop.h"


static void
test_phosh_search_result_new (void)
{
  g_autoptr (PhoshSearchResult) result = NULL;
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (PhoshSearchResultMeta) meta_dest = NULL;
  g_autoptr (GIcon) icon = NULL;

  icon = g_themed_icon_new ("start-here");

  meta = phosh_search_result_meta_new ("test",
                                       "Test",
                                       "Result",
                                       icon,
                                       "copy-me");
  g_assert_nonnull (meta);

  result = phosh_search_result_new (meta);
  g_assert_nonnull (result);

  g_object_get (result, "data", &meta_dest, NULL);

  g_assert_true (meta == meta_dest);
}


static void
test_phosh_search_result_get (void)
{
  g_autoptr (PhoshSearchResult) result = NULL;
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autofree char *id = NULL;
  g_autofree char *title = NULL;
  g_autofree char *desc = NULL;
  g_autoptr (GIcon) icon_dest = NULL;
  g_autofree char *clipboard_text = NULL;

  icon = g_themed_icon_new ("start-here");

  meta = phosh_search_result_meta_new ("test",
                                       "Test",
                                       "Result",
                                       icon,
                                       "copy-me");
  g_assert_nonnull (meta);

  result = phosh_search_result_new (meta);
  g_assert_nonnull (result);

  g_object_get (result,
                "id", &id,
                "title", &title,
                "description", &desc,
                "icon", &icon_dest,
                "clipboard-text", &clipboard_text,
                NULL);

  g_assert_cmpstr (id, ==, phosh_search_result_meta_get_id (meta));
  g_assert_cmpstr (title, ==, phosh_search_result_meta_get_title (meta));
  g_assert_cmpstr (desc, ==, phosh_search_result_meta_get_description (meta));
  g_assert_true (g_icon_equal (icon_dest,
                               phosh_search_result_meta_get_icon (meta)));
  g_assert_cmpstr (clipboard_text,
                   ==,
                   phosh_search_result_meta_get_clipboard_text (meta));

  BAD_PROP (result, phosh_search_result, PhoshSearchResult);
}


static void
test_phosh_search_result_get_accessors (void)
{
  g_autoptr (PhoshSearchResult) result = NULL;
  g_autoptr (PhoshSearchResultMeta) meta = NULL;
  g_autoptr (GIcon) icon = NULL;

  icon = g_themed_icon_new ("start-here");

  meta = phosh_search_result_meta_new ("test",
                                       "Test",
                                       "Result",
                                       icon,
                                       "copy-me");
  g_assert_nonnull (meta);

  result = phosh_search_result_new (meta);
  g_assert_nonnull (result);

  g_assert_cmpstr (phosh_search_result_get_id (result),
                   ==,
                   phosh_search_result_meta_get_id (meta));
  g_assert_cmpstr (phosh_search_result_get_title (result),
                   ==,
                   phosh_search_result_meta_get_title (meta));
  g_assert_cmpstr (phosh_search_result_get_description (result),
                   ==,
                   phosh_search_result_meta_get_description (meta));
  g_assert_true (g_icon_equal (phosh_search_result_get_icon (result),
                               phosh_search_result_meta_get_icon (meta)));
  g_assert_cmpstr (phosh_search_result_get_clipboard_text (result),
                   ==,
                   phosh_search_result_meta_get_clipboard_text (meta));
}


static void
test_phosh_search_result_null_instance (void)
{
  g_autoptr (PhoshSearchResult) result = NULL;

  NULL_INSTANCE_CALL_RETURN (phosh_search_result_get_id,
                             "PHOSH_IS_SEARCH_RESULT (self)",
                             NULL);
  NULL_INSTANCE_CALL_RETURN (phosh_search_result_get_title,
                             "PHOSH_IS_SEARCH_RESULT (self)",
                             NULL);
  NULL_INSTANCE_CALL_RETURN (phosh_search_result_get_description,
                             "PHOSH_IS_SEARCH_RESULT (self)",
                             NULL);
  NULL_INSTANCE_CALL_RETURN (phosh_search_result_get_icon,
                             "PHOSH_IS_SEARCH_RESULT (self)",
                             NULL);
  NULL_INSTANCE_CALL_RETURN (phosh_search_result_get_clipboard_text,
                             "PHOSH_IS_SEARCH_RESULT (self)",
                             NULL);
}


int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/search-result/new",
                   test_phosh_search_result_new);
  g_test_add_func ("/phosh/search-result/get",
                   test_phosh_search_result_get);
  g_test_add_func ("/phosh/search-result/get/accessors",
                   test_phosh_search_result_get_accessors);
  g_test_add_func ("/phosh/search-result/null-instance",
                   test_phosh_search_result_null_instance);

  return g_test_run ();
}

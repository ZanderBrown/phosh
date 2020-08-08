/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define TESTING_APP_SEARCH
#include "app-search/app-search.c"


static void
test_phosh_app_search_main (void)
{
  int res = real_main (0, NULL);

  g_assert_cmpint (res, ==, 0);
}


int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phosh/app-search/main",
                   test_phosh_app_search_main);

  return g_test_run ();
}

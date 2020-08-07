/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include <glib.h>


/**
 * NULL_INSTANCE_CALL:
 * @func: the function to call
 * @...: arguments to @func
 *
 * Attempt call @func on %NULL, expecting a log in @G_LOG_DOMAIN
 *
 * Used to check the call guards in methods
 */
#define NULL_INSTANCE_CALL(func, ...)                                         \
  g_test_expect_message (G_LOG_DOMAIN,                                        \
                         G_LOG_LEVEL_CRITICAL,                                \
                         #func ": assertion 'self != NULL' failed");          \
  func (NULL, ##__VA_ARGS__);

/**
 * NULL_INSTANCE_CALL_RETURN:
 * @func: the function to call
 * @ret: the return value expected
 * @...: arguments to @func
 *
 * Attempt call @func on %NULL, expecting a log in @G_LOG_DOMAIN
 *
 * Used to check the call guards in methods
 */
#define NULL_INSTANCE_CALL_RETURN(func, ret, ...)                             \
  g_test_expect_message (G_LOG_DOMAIN,                                        \
                         G_LOG_LEVEL_CRITICAL,                                \
                         #func ": assertion 'self != NULL' failed");          \
  g_assert_true (func (NULL, ##__VA_ARGS__) == ret);

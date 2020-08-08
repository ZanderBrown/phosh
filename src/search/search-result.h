/*
 * Copyright © 2019-2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include <gio/gio.h>

#include "search-result-meta.h"

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_SEARCH_RESULT (phosh_search_result_get_type ())

G_DECLARE_FINAL_TYPE (PhoshSearchResult, phosh_search_result, PHOSH, SEARCH_RESULT, GObject)


PhoshSearchResult *phosh_search_result_new                (PhoshSearchResultMeta *data);
const char        *phosh_search_result_get_id             (PhoshSearchResult     *self);
const char        *phosh_search_result_get_title          (PhoshSearchResult     *self);
const char        *phosh_search_result_get_description    (PhoshSearchResult     *self);
GIcon             *phosh_search_result_get_icon           (PhoshSearchResult     *self);
const char        *phosh_search_result_get_clipboard_text (PhoshSearchResult     *self);


G_END_DECLS

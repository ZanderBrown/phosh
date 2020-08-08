/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#pragma once

#include <gtk/gtk.h>

#include "search-result.h"

G_BEGIN_DECLS


#define PHOSH_TYPE_SEARCH_RESULT_CONTENT (phosh_search_result_content_get_type ())


G_DECLARE_FINAL_TYPE (PhoshSearchResultContent, phosh_search_result_content, PHOSH, SEARCH_RESULT_CONTENT, GtkListBoxRow)


GtkWidget         *phosh_search_result_content_new          (PhoshSearchResult        *result);
PhoshSearchResult *phosh_search_result_content_get_result   (PhoshSearchResultContent *self);


G_END_DECLS

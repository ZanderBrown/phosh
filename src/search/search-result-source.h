/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#pragma once

#include <glib-object.h>

#include "search-result.h"
#include "search-source.h"

G_BEGIN_DECLS


#define PHOSH_TYPE_SEARCH_RESULT_SOURCE (phosh_search_result_source_get_type ())


G_DECLARE_FINAL_TYPE (PhoshSearchResultSource, phosh_search_result_source, PHOSH, SEARCH_RESULT_SOURCE, GObject)


PhoshSearchResultSource *phosh_search_result_source_new               (PhoshSearchSource       *source);
void                     phosh_search_result_source_add               (PhoshSearchResultSource *self,
                                                                       PhoshSearchResult       *result);
PhoshSearchSource       *phosh_search_result_source_get_source        (PhoshSearchResultSource *self);
const char              *phosh_search_result_source_get_id            (PhoshSearchResultSource *self);
GAppInfo                *phosh_search_result_source_get_app_info      (PhoshSearchResultSource *self);
guint                    phosh_search_result_source_get_total_results (PhoshSearchResultSource *self);

G_END_DECLS

/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#pragma once

#include <glib-object.h>

#include "search-result-source.h"

G_BEGIN_DECLS


#define PHOSH_TYPE_SEARCH_RESULT_LIST (phosh_search_result_list_get_type ())


G_DECLARE_FINAL_TYPE (PhoshSearchResultList, phosh_search_result_list, PHOSH, SEARCH_RESULT_LIST, GObject)


PhoshSearchResultList *phosh_search_result_list_new (void);
void                   phosh_search_result_list_add (PhoshSearchResultList   *self,
                                                     PhoshSearchResultSource *source);

G_END_DECLS

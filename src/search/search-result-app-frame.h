/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#pragma once

#include <gtk/gtk.h>

#include "search-result-source.h"

G_BEGIN_DECLS


#define PHOSH_TYPE_SEARCH_RESULT_APP_FRAME phosh_search_result_app_frame_get_type ()


G_DECLARE_FINAL_TYPE (PhoshSearchResultAppFrame, phosh_search_result_app_frame, PHOSH, SEARCH_RESULT_APP_FRAME, GtkListBoxRow)


GtkWidget         *phosh_search_result_app_frame_new         (void);
void               phosh_search_result_app_frame_bind_source (PhoshSearchResultAppFrame *self,
                                                              PhoshSearchResultSource   *source);

G_END_DECLS

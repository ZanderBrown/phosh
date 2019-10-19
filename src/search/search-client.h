/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include <gio/gio.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_SEARCH_CLIENT phosh_search_client_get_type()
G_DECLARE_DERIVABLE_TYPE (PhoshSearchClient, phosh_search_client, PHOSH, SEARCH_CLIENT, GObject)

struct _PhoshSearchClientClass
{
  GObjectClass parent_class;
};

const char        *phosh_search_client_markup_string (PhoshSearchClient    *self,
                                                      const char           *string);
void               phosh_search_client_query         (PhoshSearchClient    *self,
                                                      const char           *query);
void               phosh_search_client_new           (GCancellable         *cancellable,
                                                      GAsyncReadyCallback   callback,
                                                      gpointer              user_data);
PhoshSearchClient *phosh_search_client_new_finish    (GObject              *source,
                                                      GAsyncResult         *result,
                                                      GError              **error);

G_END_DECLS

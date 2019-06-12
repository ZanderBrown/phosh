/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <gdk/gdk.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_SEARCH_PROVIDER_RESULT_META (phosh_search_provider_result_meta_get_type ())

typedef struct _PhoshSearchProviderResultMeta PhoshSearchProviderResultMeta;

GType           phosh_search_provider_result_meta_get_type           (void);
void            phosh_search_provider_result_meta_free               (gpointer                       source);
const char     *phosh_search_provider_result_meta_get_id             (PhoshSearchProviderResultMeta *self);
const char     *phosh_search_provider_result_meta_get_name           (PhoshSearchProviderResultMeta *self);
const char     *phosh_search_provider_result_meta_get_description    (PhoshSearchProviderResultMeta *self);
GIcon          *phosh_search_provider_result_meta_get_icon           (PhoshSearchProviderResultMeta *self);
const char     *phosh_search_provider_result_meta_get_clipboard_text (PhoshSearchProviderResultMeta *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhoshSearchProviderResultMeta,
                               phosh_search_provider_result_meta_free)

#define PHOSH_TYPE_SEARCH_PROVIDER phosh_search_provider_get_type()
G_DECLARE_DERIVABLE_TYPE (PhoshSearchProvider, phosh_search_provider, PHOSH, SEARCH_PROVIDER, GObject)

struct _PhoshSearchProviderClass
{
  GObjectClass parent_class;
};

PhoshSearchProvider *phosh_search_provider_new                    (const char          *desktop_app_id,
                                                                   const char          *bus_path,
                                                                   const char          *bus_name,
                                                                   gboolean             autostart,
                                                                   gboolean             default_disabled);
void                 phosh_search_provider_activate_result        (PhoshSearchProvider *self,
                                                                   const char          *result,
                                                                   const char *const   *terms);
void                 phosh_search_provider_launch                 (PhoshSearchProvider *self,
                                                                   const char *const   *terms);
void                 phosh_search_provider_get_result_meta        (PhoshSearchProvider *self,
                                                                   const char *const   *results,
                                                                   GAsyncReadyCallback  callback,
                                                                   gpointer             callback_data);
GPtrArray           *phosh_search_provider_get_result_meta_finish (PhoshSearchProvider  *self,
                                                                   GAsyncResult         *res,
                                                                   GError              **error);
void                 phosh_search_provider_get_initial            (PhoshSearchProvider *self,
                                                                   const char *const   *terms,
                                                                   GAsyncReadyCallback  callback,
                                                                   gpointer             callback_data);
GStrv                phosh_search_provider_get_initial_finish     (PhoshSearchProvider  *self,
                                                                   GAsyncResult         *res,
                                                                   GError              **error);
void                 phosh_search_provider_get_subsearch          (PhoshSearchProvider *self,
                                                                   const char *const   *results,
                                                                   const char *const   *terms,
                                                                   GAsyncReadyCallback  callback,
                                                                   gpointer             callback_data);
GStrv                phosh_search_provider_get_subsearch_finish   (PhoshSearchProvider  *self,
                                                                   GAsyncResult         *res,
                                                                   GError              **error);

G_END_DECLS

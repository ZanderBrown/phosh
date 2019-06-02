/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_SEARCH_PROVIDER phosh_search_provider_get_type()
G_DECLARE_DERIVABLE_TYPE (PhoshSearchProvider, phosh_search_provider, PHOSH, SEARCH_PROVIDER, GObject)

struct _PhoshSearchProviderClass
{
  GObjectClass parent_class;
};


G_END_DECLS

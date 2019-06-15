/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#include <gio/gio.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_SEARCH phosh_search_get_type()
G_DECLARE_DERIVABLE_TYPE (PhoshSearch, phosh_search, PHOSH, SEARCH, GObject)

struct _PhoshSearchClass
{
  GObjectClass parent_class;
};

void        phosh_search_set_terms     (PhoshSearch *self,
                                        GStrv        terms);
const char *phosh_search_markup_string (PhoshSearch *self,
                                        const char  *str);

G_END_DECLS

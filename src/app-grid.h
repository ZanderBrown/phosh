/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include <handy.h>

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_APP_GRID phosh_app_grid_get_type()
G_DECLARE_DERIVABLE_TYPE (PhoshAppGrid, phosh_app_grid, PHOSH, APP_GRID, GtkBox)

struct _PhoshAppGridClass
{
  GtkBoxClass parent_class;
};

void phosh_app_grid_reset (PhoshAppGrid *self);
GtkWidget *phosh_app_grid_new (void);

G_END_DECLS

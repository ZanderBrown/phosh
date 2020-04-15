/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#pragma once

#include <gtk/gtk.h>

void phosh_cp_widget_destroy (void *widget);
gchar* phosh_fix_app_id (const gchar* app_id);
void phosh_clear_handler (gulong *handler, gpointer object);

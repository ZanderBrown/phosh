/*
 * Copyright (C) 2018 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */
#pragma once

#include "monitor/phosh-display-dbus.h"
#include "monitor/monitor.h"
#include <glib-object.h>

#define PHOSH_TYPE_MONITOR_MANAGER                 (phosh_monitor_manager_get_type ())
G_DECLARE_FINAL_TYPE (PhoshMonitorManager, phosh_monitor_manager, PHOSH, MONITOR_MANAGER,
                      PhoshDisplayDbusDisplayConfigSkeleton)

PhoshMonitorManager * phosh_monitor_manager_new                       (void);
void                  phosh_monitor_manager_add_monitor               (PhoshMonitorManager *self,
                                                                       PhoshMonitor        *monitor);
PhoshMonitor        * phosh_monitor_manager_get_monitor               (PhoshMonitorManager *self,
                                                                       guint                num);
guint                 phosh_monitor_manager_get_num_monitors          (PhoshMonitorManager *self);
PhoshMonitor        * phosh_monitor_manager_find_monitor              (PhoshMonitorManager *self,
                                                                       const char          *name);

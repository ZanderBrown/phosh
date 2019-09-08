/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-overview"

#include "config.h"

#include "overview.h"
#include "activity.h"
#include "app-grid.h"
#include "app-grid-button.h"
#include "shell.h"
#include "util.h"
#include "toplevel-manager.h"
#include "phosh-private-client-protocol.h"
#include "phosh-wayland.h"

#include <gio/gdesktopappinfo.h>

#define OVERVIEW_ICON_SIZE 64

enum {
  ACTIVITY_LAUNCHED,
  ACTIVITY_RAISED,
  ACTIVITY_CLOSED,
  SELECTION_ABORTED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

typedef struct
{
  /* Running activities */
  GtkWidget *evbox_running_activities;
  GtkWidget *box_running_activities;

  GtkWidget *app_grid;
} PhoshOverviewPrivate;


struct _PhoshOverview
{
  GtkBoxClass parent;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshOverview, phosh_overview, GTK_TYPE_BOX)


static void
on_activity_clicked (PhoshOverview *self, PhoshActivity *activity)
{
  PhoshToplevel *toplevel;
  g_return_if_fail (PHOSH_IS_OVERVIEW (self));
  g_return_if_fail (PHOSH_IS_ACTIVITY (activity));

  toplevel = g_object_get_data (G_OBJECT (activity), "toplevel");
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));

  g_debug("Will raise %s (%s)",
          phosh_activity_get_app_id (activity),
          phosh_activity_get_title (activity));

  phosh_toplevel_raise (toplevel, phosh_wayland_get_wl_seat (phosh_wayland_get_default ()));
  g_signal_emit (self, signals[ACTIVITY_RAISED], 0);
}


static void
on_activity_close_clicked (PhoshOverview *self, PhoshActivity *activity)
{
  PhoshToplevel *toplevel;
  g_return_if_fail (PHOSH_IS_OVERVIEW (self));
  g_return_if_fail (PHOSH_IS_ACTIVITY (activity));

  toplevel = g_object_get_data (G_OBJECT (activity), "toplevel");
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));

  g_debug ("Will close %s (%s)",
           phosh_activity_get_app_id (activity),
           phosh_activity_get_title (activity));

  phosh_toplevel_close (toplevel);
  g_signal_emit (self, signals[ACTIVITY_CLOSED], 0);
}

static void
on_toplevel_closed (PhoshToplevel *toplevel, PhoshActivity *activity)
{
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));
  g_return_if_fail (PHOSH_IS_ACTIVITY (activity));
  gtk_widget_destroy (GTK_WIDGET (activity));
}

static void
add_activity (PhoshOverview *self, PhoshToplevel *toplevel)
{
  PhoshMonitor *monitor = phosh_shell_get_primary_monitor (phosh_shell_get_default ());
  PhoshOverviewPrivate *priv;
  GtkWidget *activity;
  const gchar *app_id, *title;

  g_return_if_fail (PHOSH_IS_OVERVIEW (self));
  priv = phosh_overview_get_instance_private (self);

  app_id = phosh_toplevel_get_app_id (toplevel);
  title = phosh_toplevel_get_title (toplevel);

  g_debug ("Building activator for '%s' (%s)", app_id, title);
  activity = phosh_activity_new (app_id, title);
  g_object_set (activity,
                "win-width", monitor->width,  // TODO: Get the real size somehow
                "win-height", monitor->height,
                NULL);
  g_object_set_data (G_OBJECT (activity), "toplevel", toplevel);
  gtk_box_pack_end (GTK_BOX (priv->box_running_activities), activity, FALSE, FALSE, 0);
  gtk_widget_show (activity);

  g_signal_connect_swapped (activity, "clicked", G_CALLBACK (on_activity_clicked), self);
  g_signal_connect_swapped (activity, "close-clicked",
                            G_CALLBACK (on_activity_close_clicked), self);

  g_signal_connect_object (toplevel, "closed", G_CALLBACK (on_toplevel_closed), activity, 0);
}

static void
get_running_activities (PhoshOverview *self)
{
  PhoshOverviewPrivate *priv;
  PhoshToplevelManager *toplevel_manager = phosh_shell_get_toplevel_manager (phosh_shell_get_default ());
  guint toplevels_num = phosh_toplevel_manager_get_num_toplevels (toplevel_manager);
  g_return_if_fail (PHOSH_IS_OVERVIEW (self));
  priv = phosh_overview_get_instance_private (self);

  if (toplevels_num == 0)
    gtk_widget_hide (priv->evbox_running_activities);

  for (guint i = 0; i < toplevels_num; i++) {
    PhoshToplevel *toplevel = phosh_toplevel_manager_get_toplevel (toplevel_manager, i);
    add_activity (self, toplevel);
  }
}

static void
toplevel_added_cb (PhoshOverview        *self,
                   PhoshToplevel        *toplevel,
                   PhoshToplevelManager *manager)
{
  g_return_if_fail (PHOSH_IS_OVERVIEW (self));
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));
  g_return_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (manager));
  add_activity (self, toplevel);
}

static void
num_toplevels_cb (PhoshOverview        *self,
                   GParamSpec *pspec,
                   PhoshToplevelManager *manager)
{
  PhoshOverviewPrivate *priv;
  g_return_if_fail (PHOSH_IS_OVERVIEW (self));
  g_return_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (manager));
  priv = phosh_overview_get_instance_private (self);
  if (phosh_toplevel_manager_get_num_toplevels (manager)) {
    gtk_widget_show (priv->evbox_running_activities);
  } else {
    gtk_widget_hide (priv->evbox_running_activities);
  }
}


static void
phosh_overview_size_allocate (GtkWidget     *widget,
                              GtkAllocation *alloc)
{
  PhoshOverview *self = PHOSH_OVERVIEW (widget);
  PhoshOverviewPrivate *priv = phosh_overview_get_instance_private (self);
  GList *children, *l;

  children = gtk_container_get_children (GTK_CONTAINER (priv->box_running_activities));

  for (l = children; l; l = l->next) {
    g_object_set (l->data,
                  "win-width", alloc->width,
                  "win-height", alloc->height,
                  NULL);
  }

  g_list_free (children);

  GTK_WIDGET_CLASS (phosh_overview_parent_class)->size_allocate (widget, alloc);
}

static void
app_launched_cb (PhoshOverview *self,
                 GAppInfo      *info,
                 GtkWidget     *widget)
{
  g_return_if_fail (PHOSH_IS_OVERVIEW (self));

  g_signal_emit (self, signals[ACTIVITY_LAUNCHED], 0);
}


static gboolean
evbox_button_press_event_cb (PhoshOverview *self, GdkEventButton *event)
{
  g_signal_emit (self, signals[SELECTION_ABORTED], 0);
  return FALSE;
}


static void
phosh_overview_constructed (GObject *object)
{
  PhoshOverview *self = PHOSH_OVERVIEW (object);
  PhoshOverviewPrivate *priv = phosh_overview_get_instance_private (self);
  PhoshToplevelManager *toplevel_manager =
      phosh_shell_get_toplevel_manager (phosh_shell_get_default ());

  G_OBJECT_CLASS (phosh_overview_parent_class)->constructed (object);

  /* Close on click */
  g_signal_connect_swapped (priv->evbox_running_activities, "button_press_event",
                            G_CALLBACK (evbox_button_press_event_cb),
                            self);
  gtk_widget_set_events (priv->evbox_running_activities, GDK_BUTTON_PRESS_MASK);

  g_signal_connect_object (toplevel_manager, "toplevel-added",
                           G_CALLBACK (toplevel_added_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (toplevel_manager, "notify::num-toplevels",
                           G_CALLBACK (num_toplevels_cb),
                           self,
                           G_CONNECT_SWAPPED);

  get_running_activities (self);

  g_signal_connect_swapped (priv->app_grid, "app-launched",
                            G_CALLBACK (app_launched_cb), self);
}


static void
phosh_overview_class_init (PhoshOverviewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_overview_constructed;
  widget_class->size_allocate = phosh_overview_size_allocate;

  gtk_widget_class_set_css_name (widget_class, "phosh-overview");

  /* ensure used custom types */
  PHOSH_TYPE_APP_GRID;
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/overview.ui");

  gtk_widget_class_bind_template_child_private (widget_class, PhoshOverview, evbox_running_activities);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshOverview, box_running_activities);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshOverview, app_grid);

  gtk_widget_class_bind_template_callback (widget_class, evbox_button_press_event_cb);

  signals[ACTIVITY_LAUNCHED] = g_signal_new ("activity-launched",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
  signals[ACTIVITY_RAISED] = g_signal_new ("activity-raised",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
  signals[SELECTION_ABORTED] = g_signal_new ("selection-aborted",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);
  signals[ACTIVITY_CLOSED] = g_signal_new ("activity-closed",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_css_name (widget_class, "phosh-overview");
}


static void
phosh_overview_init (PhoshOverview *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


void
phosh_overview_reset (PhoshOverview *self)
{
  PhoshOverviewPrivate *priv;
  g_return_if_fail(PHOSH_IS_OVERVIEW (self));
  priv = phosh_overview_get_instance_private (self);
  phosh_app_grid_reset (PHOSH_APP_GRID (priv->app_grid));
}


GtkWidget *
phosh_overview_new (void)
{
  return g_object_new (PHOSH_TYPE_OVERVIEW, NULL);
}


gboolean
phosh_overview_handle_event (PhoshOverview *self,
                             GdkEvent       *event)
{
  PhoshOverviewPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_OVERVIEW (self), GDK_EVENT_PROPAGATE);

  priv = phosh_overview_get_instance_private (self);

  return phosh_app_grid_handle_event (PHOSH_APP_GRID (priv->app_grid),
                                      event);
}

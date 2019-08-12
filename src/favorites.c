/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Based on maynard's favorites whish is
 * Copyright (C) 2013 Collabora Ltd.
 * Author: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#define G_LOG_DOMAIN "phosh-favorites"

#include "config.h"

#include "favorites.h"
#include "activity.h"
#include "app-grid.h"
#include "app-grid-button.h"
#include "shell.h"
#include "util.h"
#include "toplevel-manager.h"
#include "phosh-private-client-protocol.h"
#include "phosh-wayland.h"

#include <gio/gdesktopappinfo.h>

#define FAVORITES_ICON_SIZE 64

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
  /* Favorites */
  GtkWidget *evbox_favorites;
  GtkWidget *fb_favorites;

  /* Running activities */
  GtkWidget *evbox_running_activities;
  GtkWidget *box_running_activities;

  GtkWidget *app_grid;

} PhoshFavoritesPrivate;


struct _PhoshFavorites
{
  GtkBoxClass parent;
};

G_DEFINE_TYPE_WITH_PRIVATE(PhoshFavorites, phosh_favorites, GTK_TYPE_BOX)


static void
on_activity_clicked (PhoshFavorites *self, PhoshActivity *activity)
{
  PhoshToplevel *toplevel;
  g_return_if_fail (PHOSH_IS_FAVORITES (self));
  g_return_if_fail (PHOSH_IS_ACTIVITY (activity));

  toplevel = g_object_get_data(G_OBJECT (activity), "toplevel");
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));

  g_debug("Will raise %s (%s)",
          phosh_activity_get_app_id (activity),
          phosh_activity_get_title (activity));

  phosh_toplevel_raise (toplevel, phosh_wayland_get_wl_seat (phosh_wayland_get_default ()));
  g_signal_emit(self, signals[ACTIVITY_RAISED], 0);
}


static void
on_activity_close_clicked (PhoshFavorites *self, PhoshActivity *activity)
{
  PhoshToplevel *toplevel;
  g_return_if_fail (PHOSH_IS_FAVORITES (self));
  g_return_if_fail (PHOSH_IS_ACTIVITY (activity));

  toplevel = g_object_get_data(G_OBJECT (activity), "toplevel");
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));

  g_debug("Will close %s (%s)",
          phosh_activity_get_app_id (activity),
          phosh_activity_get_title (activity));

  phosh_toplevel_close (toplevel);
  g_signal_emit(self, signals[ACTIVITY_CLOSED], 0);
}

static void
on_toplevel_closed (PhoshToplevel *toplevel, PhoshActivity *activity)
{
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));
  g_return_if_fail (PHOSH_IS_ACTIVITY (activity));
  gtk_widget_destroy (GTK_WIDGET (activity));
}

static void add_activity (PhoshFavorites *self, PhoshToplevel *toplevel)
{
  PhoshMonitor *monitor = phosh_shell_get_primary_monitor (phosh_shell_get_default ());
  PhoshFavoritesPrivate *priv;
  GtkWidget *activity;
  const gchar *app_id, *title;

  g_return_if_fail (PHOSH_IS_FAVORITES (self));
  priv = phosh_favorites_get_instance_private (self);

  app_id = phosh_toplevel_get_app_id (toplevel);
  title = phosh_toplevel_get_title (toplevel);

  g_debug ("Building activator for '%s' (%s)", app_id, title);
  activity = phosh_activity_new (app_id, title);
  g_object_set (activity,
                "win-width", monitor->width,  // TODO: Get the real size somehow
                "win-height", monitor->height,
                NULL);
  g_object_set_data (G_OBJECT(activity), "toplevel", toplevel);
  gtk_box_pack_end (GTK_BOX (priv->box_running_activities), activity, FALSE, FALSE, 0);
  gtk_widget_show (activity);

  g_signal_connect_swapped (activity, "clicked", G_CALLBACK (on_activity_clicked), self);
  g_signal_connect_swapped (activity, "close-clicked",
                            G_CALLBACK (on_activity_close_clicked), self);

  g_signal_connect_object (toplevel, "closed", G_CALLBACK (on_toplevel_closed), activity, 0);
}

static void
get_running_activities (PhoshFavorites *self)
{
  PhoshToplevelManager *toplevel_manager = phosh_shell_get_toplevel_manager (phosh_shell_get_default ());
  guint toplevels_num = phosh_toplevel_manager_get_num_toplevels (toplevel_manager);

  for (guint i = 0; i < toplevels_num; i++) {
    PhoshToplevel *toplevel = phosh_toplevel_manager_get_toplevel (toplevel_manager, i);
    add_activity (self, toplevel);
  }
}

static void
toplevel_added_cb (PhoshFavorites *self, PhoshToplevel *toplevel, PhoshToplevelManager *manager)
{
  g_return_if_fail (PHOSH_IS_FAVORITES (self));
  g_return_if_fail (PHOSH_IS_TOPLEVEL (toplevel));
  g_return_if_fail (PHOSH_IS_TOPLEVEL_MANAGER (manager));
  add_activity (self, toplevel);
}

static void
phosh_favorites_size_allocate (GtkWidget     *widget,
                               GtkAllocation *alloc)
{
  PhoshFavorites *self = PHOSH_FAVORITES (widget);
  PhoshFavoritesPrivate *priv = phosh_favorites_get_instance_private (self);
  GList *children, *l;

  children = gtk_container_get_children (GTK_CONTAINER (priv->box_running_activities));

  for (l = children; l; l = l->next)
    g_object_set (l->data,
                  "win-width", alloc->width,
                  "win-height", alloc->height,
                  NULL);

  g_list_free (children);

  GTK_WIDGET_CLASS (phosh_favorites_parent_class)->size_allocate (widget, alloc);
}

static void
app_launched_cb (PhoshFavorites *self,
                 GAppInfo       *info,
                 GtkWidget      *widget)
{
  g_return_if_fail (PHOSH_IS_FAVORITES (self));

  g_signal_emit (self, signals[ACTIVITY_LAUNCHED], 0);
}


static gboolean
evbox_button_press_event_cb (PhoshFavorites *self, GdkEventButton *event)
{
  g_signal_emit(self, signals[SELECTION_ABORTED], 0);
  return FALSE;
}


static void
phosh_favorites_constructed (GObject *object)
{
  PhoshFavorites *self = PHOSH_FAVORITES (object);
  PhoshFavoritesPrivate *priv = phosh_favorites_get_instance_private (self);
  PhoshToplevelManager *toplevel_manager =
      phosh_shell_get_toplevel_manager (phosh_shell_get_default ());

  G_OBJECT_CLASS (phosh_favorites_parent_class)->constructed (object);

  /* Close on click */
  g_signal_connect_swapped (priv->evbox_favorites, "button_press_event",
                            G_CALLBACK (evbox_button_press_event_cb),
                            self);
  gtk_widget_set_events (priv->evbox_favorites, GDK_BUTTON_PRESS_MASK);

  /* Close on click */
  g_signal_connect_swapped (priv->evbox_running_activities, "button_press_event",
                            G_CALLBACK (evbox_button_press_event_cb),
                            self);
  gtk_widget_set_events (priv->evbox_running_activities, GDK_BUTTON_PRESS_MASK);

  g_signal_connect_object (toplevel_manager, "toplevel-added",
                           G_CALLBACK (toplevel_added_cb),
                           self,
                           G_CONNECT_SWAPPED);

  get_running_activities (self);

  g_signal_connect_swapped (priv->app_grid, "app-launched",
                            G_CALLBACK (app_launched_cb), self);
}


static void
phosh_favorites_class_init (PhoshFavoritesClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_favorites_constructed;
  widget_class->size_allocate = phosh_favorites_size_allocate;

  g_type_ensure (PHOSH_TYPE_APP_GRID);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/favorites.ui");

  gtk_widget_class_bind_template_child_private (widget_class, PhoshFavorites, evbox_favorites);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshFavorites, fb_favorites);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshFavorites, evbox_running_activities);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshFavorites, box_running_activities);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshFavorites, app_grid);

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

  gtk_widget_class_set_css_name (widget_class, "phosh-favorites");
}


static void
phosh_favorites_init (PhoshFavorites *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_favorites_new (void)
{
  return g_object_new (PHOSH_TYPE_FAVORITES, NULL);
}

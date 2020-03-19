/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-activity"

#include "config.h"
#include "activity.h"
#include "shell.h"
#include "util.h"
#include "app-grid-button.h"

#include <gio/gdesktopappinfo.h>

/**
 * SECTION:phosh-activity
 * @short_description: An app in the faovorites overview
 * @Title: PhoshActivity
 *
 * The #PhoshActivity is used to select a running application in the overview.
 */

// Icons actually sized according to the pixel-size set in the template
#define ACTIVITY_ICON_SIZE -1

enum {
  CLOSE_CLICKED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

enum {
  PROP_0,
  PROP_APP_ID,
  PROP_TITLE,
  PROP_WIN_WIDTH,
  PROP_WIN_HEIGHT,
  LAST_PROP,
};
static GParamSpec *props[LAST_PROP];

typedef struct
{
  GtkWidget *icon;
  GtkWidget *app_name;
  GtkWidget *box;
  GtkWidget *btn_close;

  int win_width;
  int win_height;

  char *app_id;
  char *title;
  GDesktopAppInfo *info;
} PhoshActivityPrivate;


struct _PhoshActivity
{
  GtkButton parent;
};

G_DEFINE_TYPE_WITH_PRIVATE(PhoshActivity, phosh_activity, GTK_TYPE_BUTTON)


static void
phosh_activity_set_property (GObject *object,
                             guint property_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
  PhoshActivity *self = PHOSH_ACTIVITY (object);
  PhoshActivityPrivate *priv = phosh_activity_get_instance_private(self);
  gint height, width;

  switch (property_id) {
    case PROP_APP_ID:
      g_free (priv->app_id);
      priv->app_id = g_value_dup_string (value);
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_APP_ID]);
      break;
    case PROP_TITLE:
      phosh_activity_set_title (self, g_value_get_string (value));
      break;
    case PROP_WIN_WIDTH:
      width = g_value_get_int (value);
      if (width != priv->win_width) {
        priv->win_width = width;
        gtk_widget_queue_resize (GTK_WIDGET (self));
        g_object_notify_by_pspec (G_OBJECT (self), props[PROP_WIN_WIDTH]);
      }
      break;
    case PROP_WIN_HEIGHT:
      height = g_value_get_int (value);
      if (height != priv->win_height) {
        priv->win_height = height;
        gtk_widget_queue_resize (GTK_WIDGET (self));
        g_object_notify_by_pspec (G_OBJECT (self), props[PROP_WIN_HEIGHT]);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_activity_get_property (GObject *object,
                             guint property_id,
                             GValue *value,
                             GParamSpec *pspec)
{
  PhoshActivity *self = PHOSH_ACTIVITY (object);
  PhoshActivityPrivate *priv = phosh_activity_get_instance_private(self);

  switch (property_id) {
    case PROP_APP_ID:
      g_value_set_string (value, priv->app_id);
      break;
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_WIN_WIDTH:
      g_value_set_int (value, priv->win_width);
      break;
    case PROP_WIN_HEIGHT:
      g_value_set_int (value, priv->win_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
on_btn_close_clicked (PhoshActivity *self, GtkButton *button)
{
  g_return_if_fail (PHOSH_IS_ACTIVITY (self));
  g_return_if_fail (GTK_IS_BUTTON (button));

  g_signal_emit(self, signals[CLOSE_CLICKED], 0);
}


static void
phosh_activity_constructed (GObject *object)
{
  PhoshActivity *self = PHOSH_ACTIVITY (object);
  PhoshActivityPrivate *priv = phosh_activity_get_instance_private (self);
  g_autofree gchar *desktop_id = NULL;

  desktop_id = g_strdup_printf ("%s.desktop", priv->app_id);
  g_return_if_fail (desktop_id);
  priv->info = g_desktop_app_info_new (desktop_id);

  if (!priv->info) {
    g_autofree gchar *name = phosh_fix_app_id (priv->app_id);
    g_return_if_fail (name);
    g_free (desktop_id);
    desktop_id = g_strdup_printf ("%s.desktop", name);
    g_return_if_fail (desktop_id);
    g_debug ("%s has broken app_id, should be %s", priv->app_id, desktop_id);
    priv->info = g_desktop_app_info_new (desktop_id);
  }

  if (priv->info) {
    gtk_image_set_from_gicon (GTK_IMAGE (priv->icon),
                              g_app_info_get_icon (G_APP_INFO (priv->info)),
                              ACTIVITY_ICON_SIZE);
  } else {
    gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon),
                                  PHOSH_APP_UNKNOWN_ICON,
                                  ACTIVITY_ICON_SIZE);
  }

  g_signal_connect_swapped (priv->btn_close,
                            "clicked",
                            (GCallback) on_btn_close_clicked,
                            self);

  G_OBJECT_CLASS (phosh_activity_parent_class)->constructed (object);
}


static void
phosh_activity_dispose (GObject *object)
{
  PhoshActivity *self = PHOSH_ACTIVITY (object);
  PhoshActivityPrivate *priv = phosh_activity_get_instance_private (self);

  g_clear_object (&priv->info);

  G_OBJECT_CLASS (phosh_activity_parent_class)->dispose (object);
}


static void
phosh_activity_finalize (GObject *object)
{
  PhoshActivity *self = PHOSH_ACTIVITY (object);
  PhoshActivityPrivate *priv = phosh_activity_get_instance_private (self);

  g_free (priv->app_id);
  g_free (priv->title);

  G_OBJECT_CLASS (phosh_activity_parent_class)->finalize (object);
}

static GtkSizeRequestMode
phosh_activity_get_request_mode (GtkWidget *widgte)
{
  return GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
}

static void
phosh_activity_get_preferred_height (GtkWidget *widget,
                                     int       *min,
                                     int       *nat)
{
  PhoshActivityPrivate *priv;
  int smallest = 0;
  int box_smallest = 0;

  g_return_if_fail (PHOSH_IS_ACTIVITY (widget));
  priv = phosh_activity_get_instance_private (PHOSH_ACTIVITY (widget));

  GTK_WIDGET_CLASS (phosh_activity_parent_class)->get_preferred_height (widget,
                                                                        &smallest,
                                                                        NULL);
  gtk_widget_get_preferred_width (priv->box, &box_smallest, NULL);

  smallest = MAX (smallest, box_smallest);

  if (min)
    *min = smallest;

  if (nat)
    *nat = smallest;
}

static void
phosh_activity_get_preferred_width_for_height (GtkWidget *widget,
                                               int        height,
                                               int       *min,
                                               int       *nat)
{
  PhoshActivityPrivate *priv;
  int smallest = 0;
  int box_smallest = 0;
  int size;
  double aspect_ratio;

  g_return_if_fail (PHOSH_IS_ACTIVITY (widget));
  priv = phosh_activity_get_instance_private (PHOSH_ACTIVITY (widget));

  GTK_WIDGET_CLASS (phosh_activity_parent_class)->get_preferred_width_for_height (widget,
                                                                                  height,
                                                                                  &smallest,
                                                                                  NULL);
  gtk_widget_get_preferred_width_for_height (priv->box, height, &box_smallest, NULL);

  smallest = MAX (smallest, box_smallest);

  aspect_ratio = (double) priv->win_width / priv->win_height;
  size = MAX (smallest, height * aspect_ratio);

  if (min)
    *min = size;

  if (nat)
    *nat = size;
}

static void
phosh_activity_get_preferred_height_for_width (GtkWidget *widget,
                                               int        width,
                                               int       *min,
                                               int       *nat)
{
  phosh_activity_get_preferred_height (widget, min, nat);
}

static void
phosh_activity_class_init (PhoshActivityClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_activity_constructed;
  object_class->dispose = phosh_activity_dispose;
  object_class->finalize = phosh_activity_finalize;

  object_class->set_property = phosh_activity_set_property;
  object_class->get_property = phosh_activity_get_property;

  widget_class->get_request_mode = phosh_activity_get_request_mode;
  widget_class->get_preferred_height = phosh_activity_get_preferred_height;
  widget_class->get_preferred_height_for_width = phosh_activity_get_preferred_height_for_width;
  widget_class->get_preferred_width_for_height = phosh_activity_get_preferred_width_for_height;

  props[PROP_APP_ID] =
    g_param_spec_string (
      "app-id",
      "app-id",
      "The application id",
      "",
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
      G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_TITLE] =
    g_param_spec_string (
      "title",
      "title",
      "The window's title",
      "",
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
      G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_WIN_WIDTH] =
    g_param_spec_int (
      "win-width",
      "Window Width",
      "The window's width",
      0,
      G_MAXINT,
      300,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_WIN_HEIGHT] =
    g_param_spec_int (
      "win-height",
      "Window Height",
      "The window's height",
      0,
      G_MAXINT,
      300,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  signals[CLOSE_CLICKED] = g_signal_new ("close-clicked",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/activity.ui");

  gtk_widget_class_bind_template_child_private (widget_class, PhoshActivity, app_name);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshActivity, icon);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshActivity, box);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshActivity, btn_close);

  gtk_widget_class_set_css_name (widget_class, "phosh-activity");
}


static void
phosh_activity_init (PhoshActivity *self)
{
  PhoshActivityPrivate *priv = phosh_activity_get_instance_private (self);

  gtk_widget_init_template (GTK_WIDGET (self));

  priv->win_height = 300;
  priv->win_width = 300;
}


GtkWidget *
phosh_activity_new (const char *app_id,
                    const char *title)
{
  return g_object_new (PHOSH_TYPE_ACTIVITY,
                       "app-id", app_id,
                       "title", title,
                       NULL);
}


const char *
phosh_activity_get_app_id (PhoshActivity *self)
{
  PhoshActivityPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_ACTIVITY (self), NULL);
  priv = phosh_activity_get_instance_private (self);

  return priv->app_id;
}

void
phosh_activity_set_title (PhoshActivity *self, const char *title)
{
  PhoshActivityPrivate *priv;

  g_return_if_fail (PHOSH_IS_ACTIVITY (self));
  priv = phosh_activity_get_instance_private (self);

  if (!g_strcmp0 (priv->title, title))
    return;

  g_free (priv->title);
  priv->title = g_strdup (title);
  gtk_label_set_label (GTK_LABEL (priv->app_name), priv->title);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TITLE]);
}

const char *
phosh_activity_get_title (PhoshActivity *self)
{
  PhoshActivityPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_ACTIVITY (self), NULL);
  priv = phosh_activity_get_instance_private (self);

  return priv->title;
}

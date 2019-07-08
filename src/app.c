/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phosh-app"

#include "config.h"
#include "app.h"
#include "shell.h"

#include <gio/gdesktopappinfo.h>

/**
 * SECTION:phosh-app
 * @short_description: An app in the faovorites overview
 * @Title: PhoshApp
 *
 * The #PhoshApp is used to select a running application
 * in the favorites overview.
 */

// Icons actually sized according to the pixel-size set in the template
#define APP_ICON_SIZE GTK_ICON_SIZE_DIALOG

enum {
  PROP_0,
  PROP_APP_ID,
  PROP_TITLE,
  PROP_MAX_WIDTH,
  PROP_MAX_HEIGHT,
  PROP_WIN_WIDTH,
  PROP_WIN_HEIGHT,
  LAST_PROP,
};
static GParamSpec *props[LAST_PROP];

typedef struct
{
  GtkWidget *icon;
  GtkWidget *app_name;
  GtkWidget *win_title;
  GtkWidget *box;

  int win_width;
  int win_height;
  int max_width;
  int max_height;

  char *app_id;
  char *title;
  GDesktopAppInfo *info;
} PhoshAppPrivate;


struct _PhoshApp
{
  GtkButton parent;
};

G_DEFINE_TYPE_WITH_PRIVATE(PhoshApp, phosh_app, GTK_TYPE_BUTTON)


static void
phosh_app_set_property (GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
  PhoshApp *self = PHOSH_APP (object);
  PhoshAppPrivate *priv = phosh_app_get_instance_private(self);

  switch (property_id) {
    case PROP_APP_ID:
      g_free (priv->app_id);
      priv->app_id = g_value_dup_string (value);
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_APP_ID]);
      break;
    case PROP_TITLE:
      g_free (priv->title);
      priv->title = g_value_dup_string (value);
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TITLE]);
      break;
    case PROP_WIN_WIDTH:
      priv->win_width = g_value_get_int (value);
      gtk_widget_queue_resize (GTK_WIDGET (self));
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_WIN_WIDTH]);
      break;
    case PROP_WIN_HEIGHT:
      priv->win_height = g_value_get_int (value);
      gtk_widget_queue_resize (GTK_WIDGET (self));
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_WIN_HEIGHT]);
      break;
    case PROP_MAX_WIDTH:
      priv->max_width = g_value_get_int (value);
      gtk_widget_queue_resize (GTK_WIDGET (self));
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_MAX_WIDTH]);
      break;
    case PROP_MAX_HEIGHT:
      priv->max_height = g_value_get_int (value);
      gtk_widget_queue_resize (GTK_WIDGET (self));
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_MAX_HEIGHT]);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_app_get_property (GObject *object,
                          guint property_id,
                          GValue *value,
                          GParamSpec *pspec)
{
  PhoshApp *self = PHOSH_APP (object);
  PhoshAppPrivate *priv = phosh_app_get_instance_private(self);

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
    case PROP_MAX_WIDTH:
      g_value_set_int (value, priv->max_width);
      break;
    case PROP_MAX_HEIGHT:
      g_value_set_int (value, priv->max_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
phosh_app_constructed (GObject *object)
{
  PhoshApp *self = PHOSH_APP (object);
  PhoshAppPrivate *priv = phosh_app_get_instance_private (self);
  g_autofree gchar *desktop_id = NULL;
  g_autofree gchar *name = NULL;

  desktop_id = g_strdup_printf ("%s.desktop", priv->app_id);
  g_return_if_fail (desktop_id);
  priv->info = g_desktop_app_info_new (desktop_id);

  /* For GTK+3 apps the desktop_id and the app_id often don't match
     because the app_id is incorrectly just $(basename argv[0]). If we
     detect this case (no dot in app_id and starts with
     lowercase) work around this by trying org.gnome.<capitalized
     app_id>.
  */
  if (!priv->info && strchr (priv->app_id, '.') == NULL && !g_ascii_isupper (priv->app_id[0])) {
    g_free (desktop_id);
    desktop_id = g_strdup_printf ("org.gnome.%c%s.desktop", priv->app_id[0] - 32, &(priv->app_id[1]));
    g_debug ("%s has broken app_id, should be %s", priv->app_id, desktop_id);
    priv->info = g_desktop_app_info_new (desktop_id);
  }

  if (priv->info) {
    name = g_desktop_app_info_get_locale_string (priv->info, "Name");
    gtk_label_set_label (GTK_LABEL (priv->app_name), name ? name : priv->app_id);
    gtk_image_set_from_gicon (GTK_IMAGE (priv->icon),
                              g_app_info_get_icon (G_APP_INFO (priv->info)),
                              APP_ICON_SIZE);
  } else {
    gtk_label_set_label (GTK_LABEL (priv->app_name), priv->app_id);
    gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon),
                                  "missing-image",
                                  APP_ICON_SIZE);
  }

  /* Only set a title if it's different from the applications name to
     avoid printing the same thing twice */
  if (g_strcmp0 (gtk_label_get_text(GTK_LABEL (priv->app_name)),
                 priv->title)) {
    gtk_label_set_text (GTK_LABEL (priv->win_title), priv->title);
  } else {
    gtk_label_set_text (GTK_LABEL (priv->win_title), "");
  }

  G_OBJECT_CLASS (phosh_app_parent_class)->constructed (object);
}


static void
phosh_app_dispose (GObject *object)
{
  PhoshApp *self = PHOSH_APP (object);
  PhoshAppPrivate *priv = phosh_app_get_instance_private (self);

  g_clear_object (&priv->info);

  G_OBJECT_CLASS (phosh_app_parent_class)->dispose (object);
}


static void
phosh_app_finalize (GObject *object)
{
  PhoshApp *self = PHOSH_APP (object);
  PhoshAppPrivate *priv = phosh_app_get_instance_private (self);

  g_free (priv->app_id);
  g_free (priv->title);

  G_OBJECT_CLASS (phosh_app_parent_class)->finalize (object);
}

static GtkSizeRequestMode
phosh_app_get_request_mode (GtkWidget *widgte)
{
  return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
phosh_app_get_preferred_height_for_width (GtkWidget *widget,
                                          int        width,
                                          int       *min,
                                          int       *nat)
{
  PhoshAppPrivate *priv;
  int smallest = 0;
  int size = 420;
  int box_smallest = 0;

  g_return_if_fail (PHOSH_IS_APP (widget));
  priv = phosh_app_get_instance_private (PHOSH_APP (widget));

  GTK_WIDGET_CLASS (phosh_app_parent_class)->get_preferred_height_for_width (widget,
                                                                             width,
                                                                             &smallest,
                                                                             NULL);
  gtk_widget_get_preferred_height_for_width (priv->box, width, &box_smallest, NULL);

  smallest = MAX (smallest, box_smallest);

  size = MAX ((width / ((gdouble) priv->win_width / (gdouble) priv->win_height)), smallest);

  size = MIN (size, priv->max_height);

  if (min) {
    *min = size;
  }

  if (nat) {
    *nat = size;
  }
}

static void
phosh_app_get_preferred_width (GtkWidget *widget, int *min, int *nat)
{
  PhoshAppPrivate *priv;
  int smallest = 0;
  int box_smallest = 0;
  int size = 250;

  g_return_if_fail (PHOSH_IS_APP (widget));
  priv = phosh_app_get_instance_private (PHOSH_APP (widget));

  GTK_WIDGET_CLASS (phosh_app_parent_class)->get_preferred_width (widget,
                                                                  &smallest,
                                                                  NULL);
  gtk_widget_get_preferred_width (priv->box, &box_smallest, NULL);

  smallest = MAX (smallest, box_smallest);

  size = MAX ((priv->max_height / ((gdouble) priv->win_height / (gdouble) priv->win_width)), smallest);

  size = MIN (size, priv->max_width);

  if (min) {
    *min = size;
  }

  if (nat) {
    *nat = size;
  }
}

static void
phosh_app_get_preferred_width_for_height (GtkWidget *widget,
                                          int        height,
                                          int       *min,
                                          int       *nat)
{
  phosh_app_get_preferred_width (widget, min, nat);
}

static void
phosh_app_class_init (PhoshAppClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_app_constructed;
  object_class->dispose = phosh_app_dispose;
  object_class->finalize = phosh_app_finalize;

  object_class->set_property = phosh_app_set_property;
  object_class->get_property = phosh_app_get_property;

  widget_class->get_request_mode = phosh_app_get_request_mode;
  widget_class->get_preferred_width = phosh_app_get_preferred_width;
  widget_class->get_preferred_height_for_width = phosh_app_get_preferred_height_for_width;
  widget_class->get_preferred_width_for_height = phosh_app_get_preferred_width_for_height;

  props[PROP_APP_ID] =
    g_param_spec_string (
      "app-id",
      "app-id",
      "The application id",
      "",
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_TITLE] =
    g_param_spec_string (
      "title",
      "title",
      "The window's title",
      "",
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_WIN_WIDTH] =
    g_param_spec_int (
      "win-width",
      "Window Width",
      "The window's width",
      0,
      G_MAXINT,
      300,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_WIN_HEIGHT] =
    g_param_spec_int (
      "win-height",
      "Window Height",
      "The window's height",
      0,
      G_MAXINT,
      300,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_MAX_WIDTH] =
    g_param_spec_int (
      "max-width",
      "Max Width",
      "The button's max width",
      0,
      G_MAXINT,
      300,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_MAX_HEIGHT] =
    g_param_spec_int (
      "max-height",
      "Max Height",
      "The button max height",
      0,
      G_MAXINT,
      300,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/app.ui");

  gtk_widget_class_bind_template_child_private (widget_class, PhoshApp, app_name);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshApp, win_title);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshApp, icon);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshApp, box);

  gtk_widget_class_set_css_name (widget_class, "phosh-app");
}


static void
phosh_app_init (PhoshApp *self)
{
  PhoshAppPrivate *priv = phosh_app_get_instance_private (self);

  gtk_widget_init_template (GTK_WIDGET (self));

  priv->max_height = 100;
  priv->max_width = 100;
  priv->win_height = 300;
  priv->win_width = 450;
}


GtkWidget *
phosh_app_new (const char *app_id, const char *title)
{
  return g_object_new (PHOSH_TYPE_APP,
                       "app-id", app_id,
                       "title", title,
                       NULL);
}


const char *
phosh_app_get_app_id (PhoshApp *self)
{
  PhoshAppPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_APP (self), NULL);
  priv = phosh_app_get_instance_private (self);

  return priv->app_id;
}


const char *
phosh_app_get_title (PhoshApp *self)
{
  PhoshAppPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_APP (self), NULL);
  priv = phosh_app_get_instance_private (self);

  return priv->title;
}

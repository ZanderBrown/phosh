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

enum {
  PHOSH_APP_PROP_0,
  PHOSH_APP_PROP_APP_ID,
  PHOSH_APP_PROP_TITLE,
  PHOSH_APP_PROP_LAST_PROP,
};
static GParamSpec *props[PHOSH_APP_PROP_LAST_PROP];

typedef struct
{
  GtkWidget *icon;
  GtkWidget *app_name;
  GtkWidget *win_title;

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
  case PHOSH_APP_PROP_APP_ID:
    g_free (priv->app_id);
    priv->app_id = g_value_dup_string (value);
    g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_APP_PROP_APP_ID]);
    break;
  case PHOSH_APP_PROP_TITLE:
    g_free (priv->title);
    priv->title = g_value_dup_string (value);
    g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_APP_PROP_TITLE]);
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
  case PHOSH_APP_PROP_APP_ID:
    g_value_set_string (value, priv->app_id);
    break;
  case PHOSH_APP_PROP_TITLE:
    g_value_set_string (value, priv->title);
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
                              GTK_ICON_SIZE_DIALOG);
  } else {
    gtk_label_set_label (GTK_LABEL (priv->app_name), priv->app_id);
    gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon),
                                  "missing-image",
                                  GTK_ICON_SIZE_DIALOG);
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

  props[PHOSH_APP_PROP_APP_ID] =
    g_param_spec_string (
      "app-id",
      "app-id",
      "The application id",
      "",
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PHOSH_APP_PROP_TITLE] =
    g_param_spec_string (
      "title",
      "title",
      "The window's title",
      "",
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PHOSH_APP_PROP_LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/app.ui");

  gtk_widget_class_bind_template_child_private (widget_class, PhoshApp, app_name);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshApp, win_title);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshApp, icon);

  gtk_widget_class_set_css_name (widget_class, "phosh-app");
}


static void
phosh_app_init (PhoshApp *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
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

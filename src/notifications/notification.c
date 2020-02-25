/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-notification"

#include "config.h"
#include "notification.h"
#include "phosh-enums.h"

#include <glib/gi18n-lib.h>

/**
 * SECTION:phosh-notification
 * @short_description: A notification
 * @Title: PhoshNotification
 */

enum {
  PROP_0,
  PROP_ID,
  PROP_APP_NAME,
  PROP_SUMMARY,
  PROP_BODY,
  PROP_APP_ICON,
  PROP_APP_INFO,
  PROP_IMAGE,
  PROP_ACTIONS,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];


enum {
  SIGNAL_ACTIONED,
  SIGNAL_EXPIRED,
  SIGNAL_CLOSED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


struct _PhoshNotification {
  GObject   parent;

  guint     id;
  char     *app_name;
  char     *summary;
  char     *body;
  GIcon    *icon;
  GIcon    *image;
  GAppInfo *info;
  GStrv     actions;

  gulong    timeout;
};
typedef struct _PhoshNotification PhoshNotification;


G_DEFINE_TYPE (PhoshNotification, phosh_notification, G_TYPE_OBJECT)


static void
phosh_notification_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  PhoshNotification *self = PHOSH_NOTIFICATION (object);

  switch (property_id) {
    case PROP_ID:
      phosh_notification_set_id (self, g_value_get_uint (value));
      break;
    case PROP_APP_NAME:
      phosh_notification_set_app_name (self, g_value_get_string (value));
      break;
    case PROP_SUMMARY:
      phosh_notification_set_summary (self, g_value_get_string (value));
      break;
    case PROP_BODY:
      phosh_notification_set_body (self, g_value_get_string (value));
      break;
    case PROP_APP_ICON:
      phosh_notification_set_app_icon (self, g_value_get_object (value));
      break;
    case PROP_APP_INFO:
      phosh_notification_set_app_info (self, g_value_get_object (value));
      break;
    case PROP_IMAGE:
      phosh_notification_set_image (self, g_value_get_object (value));
      break;
    case PROP_ACTIONS:
      phosh_notification_set_actions (self, g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_notification_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PhoshNotification *self = PHOSH_NOTIFICATION (object);

  switch (property_id) {
    case PROP_ID:
      g_value_set_uint (value, phosh_notification_get_id (self));
      break;
    case PROP_APP_NAME:
      g_value_set_string (value, phosh_notification_get_app_name (self));
      break;
    case PROP_SUMMARY:
      g_value_set_string (value, phosh_notification_get_summary (self));
      break;
    case PROP_BODY:
      g_value_set_string (value, phosh_notification_get_body (self));
      break;
    case PROP_APP_ICON:
      g_value_set_object (value, phosh_notification_get_app_icon (self));
      break;
    case PROP_APP_INFO:
      g_value_set_object (value, phosh_notification_get_app_info (self));
      break;
    case PROP_IMAGE:
      g_value_set_object (value, phosh_notification_get_image (self));
      break;
    case PROP_ACTIONS:
      g_value_set_boxed (value, phosh_notification_get_actions (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_notification_finalize (GObject *object)
{
  PhoshNotification *self = PHOSH_NOTIFICATION (object);

  // If we've been dismissed cancel the auto timeout
  if (self->timeout != 0) {
    g_source_remove (self->timeout);
  }

  g_clear_pointer (&self->app_name, g_free);
  g_clear_pointer (&self->summary, g_free);
  g_clear_pointer (&self->body, g_free);
  g_clear_object (&self->icon);
  g_clear_object (&self->image);
  g_clear_object (&self->info);
  g_clear_pointer (&self->actions, g_strfreev);

  G_OBJECT_CLASS (phosh_notification_parent_class)->finalize (object);
}


static void
phosh_notification_class_init (PhoshNotificationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phosh_notification_finalize;
  object_class->set_property = phosh_notification_set_property;
  object_class->get_property = phosh_notification_get_property;

  props[PROP_ID] =
    g_param_spec_uint (
      "id",
      "ID",
      "Notification ID",
      0,
      G_MAXUINT,
      0,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_APP_NAME] =
    g_param_spec_string (
      "app-name",
      "App Name",
      "The applications's name",
      "",
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_SUMMARY] =
    g_param_spec_string (
      "summary",
      "Summary",
      "The notification's summary",
      "",
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_BODY] =
    g_param_spec_string (
      "body",
      "Body",
      "The notification's body",
      "",
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_APP_ICON] =
    g_param_spec_object (
      "app-icon",
      "App Icon",
      "Application icon",
      G_TYPE_ICON,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PhoshNotification:app-info:
   *
   * When non-%NULL this overrides the values of #PhoshNotification:app-name
   * and #PhoshNotification:app-icon with those from the #GAppInfo
   */
  props[PROP_APP_INFO] =
    g_param_spec_object (
      "app-info",
      "App Info",
      "Application info",
      G_TYPE_APP_INFO,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_IMAGE] =
    g_param_spec_object (
      "image",
      "Image",
      "Notification image",
      G_TYPE_ICON,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_ACTIONS] =
    g_param_spec_boxed (
      "actions",
      "Actions",
      "Notification actions",
      G_TYPE_STRV,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  /**
   * PhoshNotifiation::actioned:
   *
   * When the user activates one of the provided actions (inc default)
   */
  signals[SIGNAL_ACTIONED] = g_signal_new ("actioned",
                                           G_TYPE_FROM_CLASS (klass),
                                           G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                           NULL,
                                           G_TYPE_NONE,
                                           1,
                                           G_TYPE_STRING);

  /**
   * PhoshNotifiation::expired:
   *
   * The timeout set by phosh_notification_expires() has expired
   */
  signals[SIGNAL_EXPIRED] = g_signal_new ("expired",
                                          G_TYPE_FROM_CLASS (klass),
                                          G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                          NULL,
                                          G_TYPE_NONE,
                                          0);

  /**
   * PhoshNotifiation::closed:
   * @self: the #PhoshNotifiation
   * @reason: why @self was closed
   *
   * The notification has been closed
   */
  signals[SIGNAL_CLOSED] = g_signal_new ("closed",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                         NULL,
                                         G_TYPE_NONE,
                                         1,
                                         PHOSH_TYPE_NOTIFICATION_REASON);
}


static void
phosh_notification_init (PhoshNotification *self)
{
  self->app_name = g_strdup (_("Notification"));
}


PhoshNotification *
phosh_notification_new (const char *app_name,
                        GAppInfo   *info,
                        const char *summary,
                        const char *body,
                        GIcon      *icon,
                        GIcon      *image,
                        GStrv       actions)
{
  return g_object_new (PHOSH_TYPE_NOTIFICATION,
                       "summary", summary,
                       "body", body,
                       "app-name", app_name,
                       "app-icon", icon,
                       // Set info after fallback name and icon
                       "app-info", info,
                       "image", image,
                       "actions", actions,
                       NULL);
}


void
phosh_notification_set_id (PhoshNotification *self,
                           guint              id)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  if (self->id == id) {
    return;
  }

  self->id = id;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ID]);
}


guint
phosh_notification_get_id (PhoshNotification *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);

  return self->id;
}


void
phosh_notification_set_app_icon (PhoshNotification *self,
                                 GIcon             *icon)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  g_clear_object (&self->icon);
  if (icon != NULL) {
    self->icon = g_object_ref (icon);
  } else {
    self->icon = g_themed_icon_new ("application-x-executable");
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_APP_ICON]);
}


GIcon *
phosh_notification_get_app_icon (PhoshNotification *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);

  if (self->info && g_app_info_get_icon (self->info)) {
    return g_app_info_get_icon (self->info);
  }

  return self->icon;
}


void
phosh_notification_set_app_info (PhoshNotification *self,
                                 GAppInfo          *info)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  g_clear_object (&self->info);

  if (info != NULL) {
    GIcon *icon;
    const char *name;

    self->info = g_object_ref (info);

    icon = g_app_info_get_icon (info);
    name = g_app_info_get_name (info);

    phosh_notification_set_app_icon (self, icon);
    phosh_notification_set_app_name (self, name);
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_APP_INFO]);
}


GAppInfo *
phosh_notification_get_app_info (PhoshNotification *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);

  return self->info;
}


void
phosh_notification_set_image (PhoshNotification *self,
                              GIcon             *image)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  if (g_set_object (&self->image, image)) {
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_IMAGE]);
  }
}


GIcon *
phosh_notification_get_image (PhoshNotification *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);

  return self->image;
}


void
phosh_notification_set_summary (PhoshNotification *self,
                                const char        *summary)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  if (g_strcmp0 (self->summary, summary) == 0) {
    return;
  }

  g_clear_pointer (&self->summary, g_free);
  self->summary = g_strdup (summary);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SUMMARY]);
}


const char *
phosh_notification_get_summary (PhoshNotification *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);

  return self->summary;
}


void
phosh_notification_set_body (PhoshNotification *self,
                             const gchar       *body)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  if (g_strcmp0 (self->body, body) == 0) {
    return;
  }

  g_clear_pointer (&self->body, g_free);
  self->body = g_strdup (body);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_BODY]);
}


const char *
phosh_notification_get_body (PhoshNotification *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);

  return self->body;
}



void
phosh_notification_set_app_name (PhoshNotification *self,
                                 const gchar       *app_name)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  if (g_strcmp0 (self->app_name, app_name) == 0) {
    return;
  }

  g_clear_pointer (&self->app_name, g_free);

  if (app_name &&
      strlen (app_name) > 0 &&
      g_strcmp0 (app_name, "notify-send") != 0) {
    self->app_name = g_strdup (app_name);
  } else {
    self->app_name = g_strdup (_("Notification"));
  }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_APP_NAME]);
}


const char *
phosh_notification_get_app_name (PhoshNotification *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);

  if (self->info && g_app_info_get_name (self->info)) {
    return g_app_info_get_name (self->info);
  }

  return self->app_name;
}



void
phosh_notification_set_actions (PhoshNotification *self,
                                GStrv              actions)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  g_clear_pointer (&self->actions, g_strfreev);
  self->actions = g_strdupv (actions);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ACTIONS]);
}


GStrv
phosh_notification_get_actions (PhoshNotification *self)
{
  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), 0);

  return self->actions;
}


void
phosh_notification_activate (PhoshNotification *self,
                             const char        *action)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  g_signal_emit (self, signals[SIGNAL_ACTIONED], 0, action);

  phosh_notification_close (self,
                            PHOSH_NOTIFICATION_REASON_DISMISSED);
}


static gboolean
expired (gpointer data)
{
  PhoshNotification *self = data;

  g_return_val_if_fail (PHOSH_IS_NOTIFICATION (self), G_SOURCE_REMOVE);

  g_debug ("%i expired", self->id);

  self->timeout = 0;

  g_signal_emit (self, signals[SIGNAL_EXPIRED], 0);

  return G_SOURCE_REMOVE;
}


/**
 * phosh_notification_expires:
 * @self: the #PhoshNotification
 * @timeout: delay (in milliseconds)
 *
 * Set @self to expire after @timeout (from this call)
 *
 * Note doesn't close the notification, for that call
 * phosh_notification_close() is response to #PhoshNotification::expired
 */
void
phosh_notification_expires (PhoshNotification *self,
                            int                timeout)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));
  g_return_if_fail (timeout > 0);

  self->timeout = g_timeout_add (timeout, expired, self);
}


/**
 * phosh_notification_close:
 * @self: the #PhoshNotification
 * @reason: Why the notification is closing
 */
void
phosh_notification_close (PhoshNotification       *self,
                          PhoshNotificationReason  reason)
{
  g_return_if_fail (PHOSH_IS_NOTIFICATION (self));

  // No point running the timeout, we're already closing
  if (self->timeout != 0) {
    g_source_remove (self->timeout);
    self->timeout = 0;
  }

  g_signal_emit (self, signals[SIGNAL_CLOSED], 0, reason);
}

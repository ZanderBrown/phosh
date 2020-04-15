/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */
/* Modemmanager abstraction */

#define G_LOG_DOMAIN "phosh-wwan-mm"

#include "phosh-wwan-iface.h"
#include "phosh-wwan-mm.h"
#include "phosh-wwan-mm-dbus.h"
#include "util.h"

#define BUS_NAME "org.freedesktop.ModemManager1"
#define OBJECT_PATH "/org/freedesktop/ModemManager1"


typedef enum { /* From ModemManager-enums.h */
    MM_MODEM_ACCESS_TECHNOLOGY_UNKNOWN     = 0,
    MM_MODEM_ACCESS_TECHNOLOGY_POTS        = 1 << 0,
    MM_MODEM_ACCESS_TECHNOLOGY_GSM         = 1 << 1,
    MM_MODEM_ACCESS_TECHNOLOGY_GSM_COMPACT = 1 << 2,
    MM_MODEM_ACCESS_TECHNOLOGY_GPRS        = 1 << 3,
    MM_MODEM_ACCESS_TECHNOLOGY_EDGE        = 1 << 4,
    MM_MODEM_ACCESS_TECHNOLOGY_UMTS        = 1 << 5,
    MM_MODEM_ACCESS_TECHNOLOGY_HSDPA       = 1 << 6,
    MM_MODEM_ACCESS_TECHNOLOGY_HSUPA       = 1 << 7,
    MM_MODEM_ACCESS_TECHNOLOGY_HSPA        = 1 << 8,
    MM_MODEM_ACCESS_TECHNOLOGY_HSPA_PLUS   = 1 << 9,
    MM_MODEM_ACCESS_TECHNOLOGY_1XRTT       = 1 << 10,
    MM_MODEM_ACCESS_TECHNOLOGY_EVDO0       = 1 << 11,
    MM_MODEM_ACCESS_TECHNOLOGY_EVDOA       = 1 << 12,
    MM_MODEM_ACCESS_TECHNOLOGY_EVDOB       = 1 << 13,
    MM_MODEM_ACCESS_TECHNOLOGY_LTE         = 1 << 14,
} PhoshWWanMMAccessTechnology;

typedef enum { /* From ModemManager-enums.h */
    MM_MODEM_LOCK_UNKNOWN        = 0,
    MM_MODEM_LOCK_NONE           = 1,
    /* ... */
} PhoshMMModemLock;

typedef enum { /*< underscore_name=mm_modem_state >*/
    MM_MODEM_STATE_FAILED        = -1,
    MM_MODEM_STATE_UNKNOWN       = 0,
    MM_MODEM_STATE_INITIALIZING  = 1,
    MM_MODEM_STATE_LOCKED        = 2,
    /* ... */
} PhoshMMModemState;

enum {
  PHOSH_WWAN_MM_PROP_0,
  PHOSH_WWAN_MM_PROP_SIGNAL_QUALITY,
  PHOSH_WWAN_MM_PROP_ACCESS_TEC,
  PHOSH_WWAN_MM_PROP_UNLOCKED,
  PHOSH_WWAN_MM_PROP_SIM,
  PHOSH_WWAN_MM_PROP_PRESENT,
  PHOSH_WWAN_MM_PROP_LAST_PROP,
};

typedef struct
{
  PhoshMMDBusModem *proxy;
  PhoshMMDBusObjectManagerClient *manager;

  /** Signals we connect to */
  gulong manager_object_added_signal_id;
  gulong manager_object_removed_signal_id;
  gulong proxy_props_signal_id;

  gchar *object_path;
  guint signal_quality;
  const char *access_tec;
  gboolean unlocked;
  gboolean sim;
  gboolean present;
} PhoshWWanMMPrivate;


typedef struct _PhoshWWanMM
{
  GObject parent;
} PhoshWWanMM;


static void phosh_wwan_mm_interface_init (PhoshWWanInterface *iface);
G_DEFINE_TYPE_WITH_CODE (PhoshWWanMM, phosh_wwan_mm, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (PHOSH_TYPE_WWAN,
                                                phosh_wwan_mm_interface_init)
                         G_ADD_PRIVATE (PhoshWWanMM))


static void
phosh_wwan_mm_update_signal_quality (PhoshWWanMM *self)
{
  PhoshWWanMMPrivate *priv = phosh_wwan_mm_get_instance_private (self);
  GVariant *v;

  g_return_if_fail (self);
  g_return_if_fail (priv->proxy);
  v = phosh_mmdbus_modem_get_signal_quality (priv->proxy);
  if (v) {
    g_variant_get(v, "(ub)", &priv->signal_quality, NULL);
    g_object_notify (G_OBJECT (self), "signal-quality");
  }
}


static const char *
user_friendly_access_tec (guint access_tec)
{
  switch (access_tec) {
  case MM_MODEM_ACCESS_TECHNOLOGY_GSM:
  case MM_MODEM_ACCESS_TECHNOLOGY_GSM_COMPACT:
    return "2G";
  case MM_MODEM_ACCESS_TECHNOLOGY_GPRS:
    return "2.5G";
  case MM_MODEM_ACCESS_TECHNOLOGY_EDGE:
    return "2.75G";
  case MM_MODEM_ACCESS_TECHNOLOGY_UMTS:
  case MM_MODEM_ACCESS_TECHNOLOGY_HSDPA:
  case MM_MODEM_ACCESS_TECHNOLOGY_HSUPA:
  case MM_MODEM_ACCESS_TECHNOLOGY_HSUPA | MM_MODEM_ACCESS_TECHNOLOGY_HSDPA:
    return "3G";
  case MM_MODEM_ACCESS_TECHNOLOGY_HSPA:
    return "3.5G";
  case MM_MODEM_ACCESS_TECHNOLOGY_HSPA_PLUS:
    return "3.75G";
  case MM_MODEM_ACCESS_TECHNOLOGY_LTE:
    return "4G";
  case MM_MODEM_ACCESS_TECHNOLOGY_1XRTT:
  case MM_MODEM_ACCESS_TECHNOLOGY_EVDO0:
  case MM_MODEM_ACCESS_TECHNOLOGY_EVDOA:
  case MM_MODEM_ACCESS_TECHNOLOGY_EVDOB:
  default:
    return NULL;
  }
}


static void
phosh_wwan_mm_update_access_tec (PhoshWWanMM *self)
{
  PhoshWWanMMPrivate *priv = phosh_wwan_mm_get_instance_private (self);
  guint access_tec;

  g_return_if_fail (self);
  g_return_if_fail (priv->proxy);
  access_tec = phosh_mmdbus_modem_get_access_technologies (
    priv->proxy);
  priv->access_tec = user_friendly_access_tec (access_tec);
  g_debug("Access tec is %s", priv->access_tec);
  g_object_notify (G_OBJECT (self), "access-tec");
}


static void
phosh_wwan_mm_update_lock_status (PhoshWWanMM *self)
{
  PhoshWWanMMPrivate *priv = phosh_wwan_mm_get_instance_private (self);
  guint unlock_required;
  gint state;

  g_return_if_fail (self);
  g_return_if_fail (priv->proxy);
  /* Whether any kind of PIN is required */
  unlock_required = phosh_mmdbus_modem_get_unlock_required (
    priv->proxy);
  /* Whether the sim card is currently locked */
  state = phosh_mmdbus_modem_get_state (
    priv->proxy);
  priv->unlocked = !!(unlock_required == MM_MODEM_LOCK_NONE ||
                      (state != MM_MODEM_STATE_LOCKED &&
                       state != MM_MODEM_STATE_FAILED));
  g_debug ("SIM is %slocked: (%d %d)", priv->unlocked ? "un" : "", state, unlock_required);
  g_object_notify (G_OBJECT (self), "unlocked");
}


static void
phosh_wwan_mm_update_sim_status (PhoshWWanMM *self)
{
  PhoshWWanMMPrivate *priv = phosh_wwan_mm_get_instance_private (self);
  const gchar *sim;

  g_return_if_fail (self);
  g_return_if_fail (priv->proxy);
  sim = phosh_mmdbus_modem_get_sim (priv->proxy);
  g_debug ("SIM path %s", sim);
  priv->sim = !!(sim != NULL && strcmp (sim, "/"));
  g_debug ("SIM is %spresent", priv->sim ? "" : "not ");
  g_object_notify (G_OBJECT (self), "sim");
}


static void
phosh_wwan_mm_update_present (PhoshWWanMM *self, gboolean present)
{
  PhoshWWanMMPrivate *priv = phosh_wwan_mm_get_instance_private (self);

  g_return_if_fail (self);

  if (priv->present != present) {
    g_debug ("Modem is %spresent", present ? "" : "not ");
    priv->present = present;
    g_object_notify (G_OBJECT (self), "present");
  }
}


static void
dbus_props_changed_cb(PhoshMMDBusModem *proxy,
                      GVariant *changed_properties,
                      GStrv invaliated,
                      PhoshWWanMM *self)
{
  char *property;
  GVariantIter i;

  g_variant_iter_init (&i, changed_properties);
  while (g_variant_iter_next (&i, "{&sv}", &property, NULL)) {
    g_debug ("WWAN property %s changed", property);
    if (strcmp (property, "AccessTechnologies") == 0) {
      phosh_wwan_mm_update_access_tec (self);
    }
    if (strcmp (property, "SignalQuality") == 0) {
      phosh_wwan_mm_update_signal_quality (self);
    }
    if (strcmp (property, "UnlockRequired") == 0) {
      phosh_wwan_mm_update_lock_status (self);
    }
    if (strcmp (property, "State") == 0) {
      phosh_wwan_mm_update_lock_status (self);
    }
    if (strcmp (property, "Sim") == 0) {
      phosh_wwan_mm_update_sim_status (self);
    }
  }
}


static void
phosh_wwan_mm_set_property (GObject *object,
                            guint property_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
  /* All props are ro */
  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}


static void
phosh_wwan_mm_get_property (GObject *object,
                            guint property_id,
                            GValue *value,
                            GParamSpec *pspec)
{
  PhoshWWanMM *self = PHOSH_WWAN_MM (object);
  PhoshWWanMMPrivate *priv = phosh_wwan_mm_get_instance_private(self);

  switch (property_id) {
  case PHOSH_WWAN_MM_PROP_SIGNAL_QUALITY:
    g_value_set_uint (value, priv->signal_quality);
    break;
  case PHOSH_WWAN_MM_PROP_ACCESS_TEC:
    g_value_set_string (value, priv->access_tec);
    break;
  case PHOSH_WWAN_MM_PROP_UNLOCKED:
    g_value_set_boolean (value, priv->unlocked);
    break;
  case PHOSH_WWAN_MM_PROP_SIM:
    g_value_set_boolean (value, priv->sim);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
destroy_modem (PhoshWWanMM *self)
{
  PhoshWWanMMPrivate *priv = phosh_wwan_mm_get_instance_private (self);

  if (priv->proxy) {
    phosh_clear_handler (&priv->proxy_props_signal_id, priv->proxy);

    g_clear_object (&priv->proxy);
  }

  g_clear_pointer (&priv->object_path, g_free);

  phosh_wwan_mm_update_present (self, FALSE);

  priv->signal_quality = 0;
  g_object_notify (G_OBJECT (self), "signal-quality");

  priv->access_tec = NULL;
  g_object_notify (G_OBJECT (self), "access-tec");

  priv->unlocked = FALSE;
  g_object_notify (G_OBJECT (self), "unlocked");

  priv->sim = FALSE;
  g_object_notify (G_OBJECT (self), "sim");
}


static void
init_modem (PhoshWWanMM *self, const gchar *object_path)
{
  PhoshWWanMMPrivate *priv = phosh_wwan_mm_get_instance_private (self);
  g_autoptr(GError) err = NULL;

  priv->proxy = phosh_mmdbus_modem_proxy_new_for_bus_sync (
    G_BUS_TYPE_SYSTEM,
    G_DBUS_PROXY_FLAGS_NONE,
    BUS_NAME,
    object_path,
    NULL,
    &err);

  if (priv->proxy == NULL) {
    g_warning ("Can't query modem at %s: %s", object_path, err->message);
    return;
  }

  priv->object_path = g_strdup (object_path);
  if (priv->object_path == NULL) {
    g_clear_object (&priv->proxy);
    g_return_if_fail (priv->object_path);
  }

  priv->proxy_props_signal_id = g_signal_connect (priv->proxy,
                                                  "g-properties-changed",
                                                  G_CALLBACK (dbus_props_changed_cb),
                                                  self);
  phosh_wwan_mm_update_signal_quality (self);
  phosh_wwan_mm_update_access_tec (self);
  phosh_wwan_mm_update_lock_status (self);
  phosh_wwan_mm_update_sim_status (self);
  phosh_wwan_mm_update_present (self, TRUE);
}


static void
object_added_cb (PhoshWWanMM *self, GDBusObject *object, PhoshMMDBusObjectManagerClient *manager)
{
  PhoshWWanMMPrivate *priv = phosh_wwan_mm_get_instance_private (self);
  const gchar *modem_object_path;

  modem_object_path = g_dbus_object_get_object_path (object);
  g_debug ("Modem added at path: %s", modem_object_path);
  if (priv->object_path == NULL) {
    g_debug ("Tracking modem at: %s", modem_object_path);
    init_modem (self, modem_object_path);
  }
}


static void
object_removed_cb (PhoshWWanMM *self, GDBusObject *object, PhoshMMDBusObjectManagerClient *manager)
{
  PhoshWWanMMPrivate *priv = phosh_wwan_mm_get_instance_private (self);
  const gchar *modem_object_path;

  modem_object_path = g_dbus_object_get_object_path (object);
  g_debug ("Modem removed at path: %s", modem_object_path);
  if (!g_strcmp0 (modem_object_path, priv->object_path)) {
    g_debug ("Dropping modem at: %s", modem_object_path);
    destroy_modem (self);
  }
}


static void
on_mm_object_manager_created (GObject *source_object, GAsyncResult *res, PhoshWWanMM *self)
{
  PhoshWWanMMPrivate *priv = phosh_wwan_mm_get_instance_private (self);
  g_autoptr (GError) err = NULL;
  g_autolist (GDBusObject) modems = NULL;
  const gchar *modem_object_path;

  priv->manager = PHOSH_MMDBUS_OBJECT_MANAGER_CLIENT (
    phosh_mmdbus_object_manager_client_new_for_bus_finish (
      res,
      &err));

  if (priv->manager == NULL) {
    g_warning ("Failed to connect modem manager: %s", err->message);
    return;
  }

  priv->manager_object_added_signal_id =
    g_signal_connect_swapped (priv->manager,
                              "object-added",
                              G_CALLBACK (object_added_cb),
                              self);

  priv->manager_object_removed_signal_id =
  g_signal_connect_swapped (priv->manager,
                            "object-removed",
                            G_CALLBACK (object_removed_cb),
                            self);

  modems = g_dbus_object_manager_get_objects (G_DBUS_OBJECT_MANAGER (priv->manager));
  if (modems) {
    /* Look at the first modem */
    modem_object_path = g_dbus_object_get_object_path (G_DBUS_OBJECT (modems->data));
    g_debug ("modem path: %s", modem_object_path);
    init_modem (self, modem_object_path);
  } else {
    g_debug ("No modem found");
  }
}

static void
phosh_wwan_mm_constructed (GObject *object)
{
  PhoshWWanMM *self = PHOSH_WWAN_MM (object);

  G_OBJECT_CLASS (phosh_wwan_mm_parent_class)->constructed (object);

  phosh_mmdbus_object_manager_client_new_for_bus (
    G_BUS_TYPE_SYSTEM,
    G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
    BUS_NAME,
    OBJECT_PATH,
    NULL,
    (GAsyncReadyCallback)on_mm_object_manager_created,
    self);
}


static void
phosh_wwan_mm_dispose (GObject *object)
{
  PhoshWWanMM *self = PHOSH_WWAN_MM (object);
  PhoshWWanMMPrivate *priv = phosh_wwan_mm_get_instance_private (self);
  GObjectClass *parent_class = G_OBJECT_CLASS (phosh_wwan_mm_parent_class);

  destroy_modem (self);
  if (priv->manager) {
    phosh_clear_handler (&priv->manager_object_added_signal_id,
                         priv->manager);
    phosh_clear_handler (&priv->manager_object_removed_signal_id,
                         priv->manager);

    g_clear_object (&priv->manager);
  }
  g_clear_pointer (&priv->object_path, g_free);

  parent_class->dispose (object);
}


static void
phosh_wwan_mm_class_init (PhoshWWanMMClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_wwan_mm_constructed;
  object_class->dispose = phosh_wwan_mm_dispose;
  object_class->set_property = phosh_wwan_mm_set_property;
  object_class->get_property = phosh_wwan_mm_get_property;

  g_object_class_override_property (object_class,
                                    PHOSH_WWAN_MM_PROP_SIGNAL_QUALITY,
                                    "signal-quality");
  g_object_class_override_property (object_class,
                                    PHOSH_WWAN_MM_PROP_ACCESS_TEC,
                                    "access-tec");
  g_object_class_override_property (object_class,
                                    PHOSH_WWAN_MM_PROP_UNLOCKED,
                                    "unlocked");
  g_object_class_override_property (object_class,
                                    PHOSH_WWAN_MM_PROP_SIM,
                                    "sim");
  g_object_class_override_property (object_class,
                                    PHOSH_WWAN_MM_PROP_PRESENT,
                                    "present");
}


static guint
phosh_wwan_mm_get_signal_quality (PhoshWWan *phosh_wwan)
{
  PhoshWWanMM *self = PHOSH_WWAN_MM (phosh_wwan);
  PhoshWWanMMPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_WWAN_MM (self), 0);
  priv = phosh_wwan_mm_get_instance_private (self);
  return priv->signal_quality;
}


static const char*
phosh_wwan_mm_get_access_tec (PhoshWWan *phosh_wwan)
{
  PhoshWWanMM *self = PHOSH_WWAN_MM (phosh_wwan);
  PhoshWWanMMPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_WWAN_MM (self), NULL);
  priv = phosh_wwan_mm_get_instance_private (self);
  return priv->access_tec;
}

static gboolean
phosh_wwan_mm_is_unlocked (PhoshWWan *phosh_wwan)
{
  PhoshWWanMM *self = PHOSH_WWAN_MM (phosh_wwan);
  PhoshWWanMMPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_WWAN_MM (self), FALSE);
  priv = phosh_wwan_mm_get_instance_private (self);
  return priv->unlocked;
}

static gboolean
phosh_wwan_mm_has_sim (PhoshWWan *phosh_wwan)
{
  PhoshWWanMM *self = PHOSH_WWAN_MM (phosh_wwan);
  PhoshWWanMMPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_WWAN_MM (self), FALSE);
  priv = phosh_wwan_mm_get_instance_private (self);
  return priv->sim;
}


static gboolean
phosh_wwan_mm_is_present (PhoshWWan *phosh_wwan)
{
  PhoshWWanMM *self = PHOSH_WWAN_MM (phosh_wwan);
  PhoshWWanMMPrivate *priv;

  g_return_val_if_fail (PHOSH_IS_WWAN_MM (self), FALSE);
  priv = phosh_wwan_mm_get_instance_private (self);
  return priv->present;
}


static void
phosh_wwan_mm_interface_init (PhoshWWanInterface *iface)
{
  iface->get_signal_quality = phosh_wwan_mm_get_signal_quality;
  iface->get_access_tec = phosh_wwan_mm_get_access_tec;
  iface->is_unlocked = phosh_wwan_mm_is_unlocked;
  iface->has_sim = phosh_wwan_mm_has_sim;
  iface->is_present = phosh_wwan_mm_is_present;
}


static void
phosh_wwan_mm_init (PhoshWWanMM *self)
{
}


PhoshWWanMM *
phosh_wwan_mm_new (void)
{
  return g_object_new (PHOSH_TYPE_WWAN_MM, NULL);
}

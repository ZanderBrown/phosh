/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#define G_LOG_DOMAIN "phosh-app-grid"

#include "app-grid.h"
#include "app-grid-button.h"
#include "app-list-model.h"

#include "gtk-list-models/gtksortlistmodel.h"
#include "gtk-list-models/gtkfilterlistmodel.h"

typedef struct _PhoshAppGridPrivate PhoshAppGridPrivate;
struct _PhoshAppGridPrivate {
  GtkFilterListModel *model;

  GtkWidget *search;
  GtkWidget *apps;
  GtkWidget *favs;
  GtkWidget *expand_image;
  GtkWidget *scroller;

  gboolean apps_expanded;

  GSettings *settings;
};

G_DEFINE_TYPE_WITH_PRIVATE (PhoshAppGrid, phosh_app_grid, GTK_TYPE_BOX)

enum {
  PROP_0,
  PROP_APPS_EXPANDED,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };

enum {
  APP_LAUNCHED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

static void
app_launched_cb (GtkWidget    *widget,
                 GAppInfo     *info,
                 PhoshAppGrid *self)
{
  g_signal_emit (self, signals[APP_LAUNCHED], 0, info);
}

static void
favorites_changed (GSettings    *settings,
                   const gchar  *key,
                   PhoshAppGrid *self)
{
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);
  gchar **favorites = g_settings_get_strv (settings, key);
  GtkWidget *btn;

  /* Remove all favorites first */
  gtk_container_foreach (GTK_CONTAINER (priv->favs),
                         (GtkCallback) gtk_widget_destroy, NULL);

  for (gint i = 0; i < g_strv_length (favorites); i++) {
    gchar *fav = favorites[i];
    GDesktopAppInfo *info;

    info = g_desktop_app_info_new (fav);

    if (!info)
      continue;

    btn = phosh_app_grid_button_new_favorite (G_APP_INFO (info));

    g_signal_connect (btn, "app-launched",
                      G_CALLBACK (app_launched_cb), self);

    gtk_widget_show (btn);

    if (btn)
      gtk_flow_box_insert (GTK_FLOW_BOX (priv->favs), btn, -1);
  }
  g_strfreev (favorites);
}

static gint
sort_apps (gconstpointer a,
           gconstpointer b,
           gpointer      data)
{
  GAppInfo *info1 = G_APP_INFO (a);
  GAppInfo *info2 = G_APP_INFO (b);
  gchar *s1, *s2;
  gint ret;

  s1 = g_utf8_casefold (g_app_info_get_display_name (info1), -1);
  s2 = g_utf8_casefold (g_app_info_get_display_name (info2), -1);

  ret = g_strcmp0 (s1, s2);

  g_free (s1);
  g_free (s2);

  return ret;
}

static gboolean
search_apps (gpointer item, gpointer data)
{
  PhoshAppGrid *self = data;
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);
  GAppInfo *info = item;
  const char *search = NULL;
  const char *str = NULL;

  g_return_val_if_fail (priv != NULL, TRUE);
  g_return_val_if_fail (priv->search != NULL, TRUE);

  search = gtk_entry_get_text (GTK_ENTRY (priv->search));

  if (search == NULL)
    return TRUE;

  if ((str = g_app_info_get_display_name (info)) && strstr (str, search))
    return TRUE;

  if ((str = g_app_info_get_name (info)) && strstr (str, search))
    return TRUE;

  if ((str = g_app_info_get_description (info)) && strstr (str, search))
    return TRUE;

  if ((str = g_app_info_get_executable (info)) && strstr (str, search))
    return TRUE;

  if (G_IS_DESKTOP_APP_INFO (info)) {
    const char * const *kwds;
    int i = 0;

    if ((str = g_desktop_app_info_get_generic_name (G_DESKTOP_APP_INFO (info))) &&
         strstr (str, search))
      return TRUE;

    if ((str = g_desktop_app_info_get_categories (G_DESKTOP_APP_INFO (info))) &&
         strstr (str, search))
      return TRUE;

    kwds = g_desktop_app_info_get_keywords (G_DESKTOP_APP_INFO (info));

    if (kwds) {
      while ((str = kwds[i])) {
        if (strstr (str, search))
          return TRUE;
        i++;
      }
    }
  }

  return FALSE;
}

static GtkWidget *
create_launcher (gpointer item,
                 gpointer self)
{
  GtkWidget *btn = phosh_app_grid_button_new (G_APP_INFO (item));

  g_signal_connect (btn, "app-launched",
                    G_CALLBACK (app_launched_cb), self);

  gtk_widget_show (btn);

  return btn;
}

static void
launcher_scrolled (GtkAdjustment *adjustment,
                   PhoshAppGrid  *self)
{
  GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET (self));
  if (gtk_adjustment_get_value (adjustment) > 0.0) {
    gtk_style_context_add_class (context, "scrolled");
  } else {
    gtk_style_context_remove_class (context, "scrolled");
  }
}

static void
phosh_app_grid_init (PhoshAppGrid *self)
{
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);
  GtkSortListModel *sorted;
  GSimpleActionGroup *actions;
  GPropertyAction *expand_action;
  GtkAdjustment *adj;

  gtk_widget_init_template (GTK_WIDGET (self));

  actions = g_simple_action_group_new ();

  expand_action = g_property_action_new ("expand", self, "apps-expanded");
  g_action_map_add_action (G_ACTION_MAP (actions), G_ACTION (expand_action));

  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "app-grid", G_ACTION_GROUP (actions));

  priv->settings = g_settings_new ("sm.puri.phosh");
  g_signal_connect (priv->settings, "changed::favorites",
                    G_CALLBACK (favorites_changed), self);
  favorites_changed (priv->settings, "favorites", self);

  /* fill the grid with apps */
  sorted = gtk_sort_list_model_new (G_LIST_MODEL (phosh_app_list_model_get_default ()),
                                    sort_apps,
                                    NULL,
                                    NULL);
  priv->model = gtk_filter_list_model_new (G_LIST_MODEL (sorted),
                                           search_apps,
                                           self,
                                           NULL);
  gtk_flow_box_bind_model (GTK_FLOW_BOX (priv->apps), G_LIST_MODEL (priv->model),
                           create_launcher, self, NULL);

  adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->scroller));
  g_signal_connect (adj, "value-changed", G_CALLBACK (launcher_scrolled), self);
}

static void
phosh_app_grid_finalize (GObject *object)
{
  PhoshAppGrid *self = PHOSH_APP_GRID (object);
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);

  g_clear_object (&priv->model);
  g_clear_object (&priv->settings);

  G_OBJECT_CLASS (phosh_app_grid_parent_class)->finalize (object);
}

static void
phosh_app_grid_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PhoshAppGrid *self = PHOSH_APP_GRID (object);
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);

  switch (property_id) {
    case PROP_APPS_EXPANDED:
      priv->apps_expanded = g_value_get_boolean (value);
      if (priv->apps_expanded) {
        gtk_image_set_from_icon_name (GTK_IMAGE (priv->expand_image),
                                      "go-down-symbolic",
                                      GTK_ICON_SIZE_BUTTON);
      } else {
        gtk_image_set_from_icon_name (GTK_IMAGE (priv->expand_image),
                                      "go-up-symbolic",
                                      GTK_ICON_SIZE_BUTTON);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
phosh_app_grid_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  PhoshAppGrid *self = PHOSH_APP_GRID (object);
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);

  switch (property_id) {
    case PROP_APPS_EXPANDED:
      g_value_set_boolean (value, priv->apps_expanded);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static gboolean
phosh_app_grid_key_press_event (GtkWidget   *widget,
                              GdkEventKey *event)
{
  PhoshAppGrid *self = PHOSH_APP_GRID (widget);
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);

  return gtk_search_entry_handle_event (GTK_SEARCH_ENTRY (priv->search), 
                                        (GdkEvent *) event);
}

static void
search_changed (GtkSearchEntry *entry,
                PhoshAppGrid    *self)
{
  PhoshAppGridPrivate *priv = phosh_app_grid_get_instance_private (self);

  gtk_filter_list_model_refilter (priv->model);
}

static void
phosh_app_grid_class_init (PhoshAppGridClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_app_grid_finalize;
  object_class->set_property = phosh_app_grid_set_property;
  object_class->get_property = phosh_app_grid_get_property;

  pspecs[PROP_APPS_EXPANDED] =
    g_param_spec_boolean ("apps-expanded", "Apps expanded", "Showing complete app tray",
                          FALSE,
                          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  widget_class->key_press_event = phosh_app_grid_key_press_event;

  gtk_widget_class_set_template_from_resource (widget_class, "/sm/puri/phosh/ui/app-grid.ui");

  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGrid, search);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGrid, apps);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGrid, favs);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGrid, expand_image);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshAppGrid, scroller);

  gtk_widget_class_bind_template_callback (widget_class, search_changed);

  signals[APP_LAUNCHED] = g_signal_new ("app-launched",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0, NULL, NULL, NULL,
                                        G_TYPE_NONE, 1, G_TYPE_APP_INFO);

  gtk_widget_class_set_css_name (widget_class, "phosh-app-grid");
}

GtkWidget *
phosh_app_grid_new (void)
{
  return g_object_new (PHOSH_TYPE_APP_GRID, NULL);
}

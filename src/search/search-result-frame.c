/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-search-result-frame"

#include "config.h"

#include "search-result-frame.h"
#include "search-result-source.h"
#include "search-result-content.h"

#include <glib/gi18n-lib.h>


/**
 * SECTION:search-result-frame
 * @short_description: A frame containing one or more #PhoshSearchResultContent
 * @Title: PhoshSearchResultFrame
 *
 * A #PhoshSearchResultFrame is the view of the #PhoshSearchResultSource model
 * containing a #PhoshSearchResultContent for each of a
 * #PhoshSearchResultSource's #PhoshSearchResults
 */


struct _PhoshSearchResultFrame {
  GtkListBoxRow            parent;

  PhoshSearchResultSource *source;

  GBinding                *bind_name;
  GBinding                *bind_icon;
  GBinding                *bind_extra;

  GtkWidget               *items;
  GtkWidget               *name_lbl;
  GtkWidget               *extra_lbl;
  GtkWidget               *icon_img;
};
typedef struct _PhoshSearchResultFrame PhoshSearchResultFrame;


G_DEFINE_TYPE (PhoshSearchResultFrame, phosh_search_result_frame, GTK_TYPE_LIST_BOX_ROW)


static void
phosh_search_result_frame_dispose (GObject *object)
{
  PhoshSearchResultFrame *self = PHOSH_SEARCH_RESULT_FRAME (object);

  // Don't clear bindings, they are unref'd automatically

  g_clear_object (&self->source);

  G_OBJECT_CLASS (phosh_search_result_frame_parent_class)->dispose (object);
}


static void
item_activated (GtkListBox             *list,
                GtkListBoxRow          *row,
                PhoshSearchResultFrame *self)
{
  g_return_if_fail (PHOSH_IS_SEARCH_RESULT_FRAME (self));

  /* TODO */
}


static void
phosh_search_result_frame_class_init (PhoshSearchResultFrameClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_search_result_frame_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/search-result-frame.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshSearchResultFrame, items);
  gtk_widget_class_bind_template_child (widget_class, PhoshSearchResultFrame, name_lbl);
  gtk_widget_class_bind_template_child (widget_class, PhoshSearchResultFrame, extra_lbl);
  gtk_widget_class_bind_template_child (widget_class, PhoshSearchResultFrame, icon_img);

  gtk_widget_class_bind_template_callback (widget_class, item_activated);

  gtk_widget_class_set_css_name (widget_class, "phosh-search-result-frame");
}


static void
phosh_search_result_frame_init (PhoshSearchResultFrame *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


/**
 * phosh_search_result_frame_new:
 *
 * Create a #PhoshSearchResultFrame, use
 * phosh_search_result_frame_bind_source() to populate it
 *
 * Returns: (transfer float): a new #PhoshSearchResultFrame
 */
GtkWidget *
phosh_search_result_frame_new (void)
{
  return g_object_new (PHOSH_TYPE_SEARCH_RESULT_FRAME, NULL);
}


static GtkWidget *
create_row (gpointer item, gpointer data)
{
  PhoshSearchResult *result = item;

  return phosh_search_result_content_new (result);
}


static gboolean
set_name (GBinding     *binding,
          const GValue *from_value,
          GValue       *to_value,
          gpointer      user_data)
{
  GAppInfo *info = g_value_get_object (from_value);

  g_value_set_string (to_value, g_app_info_get_name (info));

  return TRUE;
}


static gboolean
set_extra (GBinding     *binding,
           const GValue *from_value,
           GValue       *to_value,
           gpointer      user_data)
{
  PhoshSearchResultFrame *self = user_data;
  guint total = g_value_get_uint (from_value);
  char *extra = NULL;
  int n = total - g_list_model_get_n_items (G_LIST_MODEL (self->source));

  if (n > 0) {
    extra = g_strdup_printf (_("%i more"), n);

    g_value_take_string (to_value, extra);
    gtk_widget_show (self->extra_lbl);
  } else {
    g_value_set_string (to_value, NULL);
    gtk_widget_hide (self->extra_lbl);
  }

  return TRUE;
}


static gboolean
set_icon (GBinding     *binding,
          const GValue *from_value,
          GValue       *to_value,
          gpointer      user_data)
{
  GAppInfo *info = g_value_get_object (from_value);

  g_value_set_object (to_value, g_app_info_get_icon (info));

  return TRUE;
}


/**
 * phosh_search_result_frame_bind_source:
 * @self: the #PhoshSearchResultFrame
 * @source: the #PhoshSearchResultSource to bind to
 *
 * Bind @self to a #PhoshSearchResultSource, note @self can be reused for
 * different #PhoshSearchResultSources
 */
void
phosh_search_result_frame_bind_source (PhoshSearchResultFrame  *self,
                                       PhoshSearchResultSource *source)
{
  g_return_if_fail (PHOSH_IS_SEARCH_RESULT_FRAME (self));
  g_return_if_fail (PHOSH_IS_SEARCH_RESULT_SOURCE (source));

  g_clear_object (&self->bind_name);
  g_clear_object (&self->bind_icon);
  g_clear_object (&self->bind_extra);

  g_set_object (&self->source, source);

  gtk_list_box_bind_model (GTK_LIST_BOX (self->items),
                           G_LIST_MODEL (source),
                           create_row,
                           self,
                           NULL);

  // Bind to the new one
  self->bind_name = g_object_bind_property_full (source,         "app-info",
                                                 self->name_lbl, "label",
                                                 G_BINDING_SYNC_CREATE,
                                                 set_name,
                                                 NULL,
                                                 self,
                                                 NULL);

  self->bind_extra = g_object_bind_property_full (source,          "total-results",
                                                  self->extra_lbl, "label",
                                                  G_BINDING_SYNC_CREATE,
                                                  set_extra,
                                                  NULL,
                                                  self,
                                                  NULL);

  self->bind_icon = g_object_bind_property_full (source,         "app-info",
                                                 self->icon_img, "gicon",
                                                 G_BINDING_SYNC_CREATE,
                                                 set_icon,
                                                 NULL,
                                                 self,
                                                 NULL);
}

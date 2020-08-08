/*
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-search-result-content"

#include "search-result-content.h"


/**
 * SECTION:search-result-content
 * @short_description: Shows a #PhoshSearchResult
 * @Title: PhoshSearchResultContent
 *
 * A #GtkListBoxRow used to represent a #PhoshSearchResult
 */

enum {
  PROP_0,
  PROP_RESULT,
  LAST_PROP
};
static GParamSpec *props[LAST_PROP];


struct _PhoshSearchResultContent {
  GtkListBoxRow      parent;

  PhoshSearchResult *result;

  GtkWidget         *icon_img;
  GtkWidget         *title_lbl;
  GtkWidget         *desc_lbl;
};
typedef struct _PhoshSearchResultContent PhoshSearchResultContent;


G_DEFINE_TYPE (PhoshSearchResultContent, phosh_search_result_content, GTK_TYPE_LIST_BOX_ROW)


static void
phosh_search_result_content_set_result (PhoshSearchResultContent *self,
                                        PhoshSearchResult        *result)
{
  g_return_if_fail (PHOSH_IS_SEARCH_RESULT_CONTENT (self));
  g_return_if_fail (PHOSH_IS_SEARCH_RESULT (result));

  g_set_object (&self->result, result);

  // Use the "transform" function to show/hide when set/unset
  g_object_bind_property (self->result,   "icon",
                          self->icon_img, "gicon",
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property (self->result,    "title",
                          self->title_lbl, "label",
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property (self->result,   "description",
                          self->desc_lbl, "label",
                          G_BINDING_SYNC_CREATE);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_RESULT]);
}


static void
phosh_search_result_content_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  PhoshSearchResultContent *self = PHOSH_SEARCH_RESULT_CONTENT (object);

  switch (property_id) {
    case PROP_RESULT:
      phosh_search_result_content_set_result (self,
                                              g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_search_result_content_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  PhoshSearchResultContent *self = PHOSH_SEARCH_RESULT_CONTENT (object);

  switch (property_id) {
    case PROP_RESULT:
      g_value_set_object (value, self->result);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_search_result_content_dispose (GObject *object)
{
  PhoshSearchResultContent *self = PHOSH_SEARCH_RESULT_CONTENT (object);

  g_clear_object (&self->result);

  G_OBJECT_CLASS (phosh_search_result_content_parent_class)->dispose (object);
}


static void
phosh_search_result_content_class_init (PhoshSearchResultContentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_search_result_content_dispose;
  object_class->set_property = phosh_search_result_content_set_property;
  object_class->get_property = phosh_search_result_content_get_property;

  /**
   * PhoshSearchResultContent:result:
   * @self: the #PhoshSearchResultContent
   *
   * The #PhoshSearchResult shown in @self
   */
  props[PROP_RESULT] =
    g_param_spec_object ("result",
                         "Result",
                         "Search result being shown",
                         PHOSH_TYPE_SEARCH_RESULT,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/search-result-content.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshSearchResultContent, icon_img);
  gtk_widget_class_bind_template_child (widget_class, PhoshSearchResultContent, title_lbl);
  gtk_widget_class_bind_template_child (widget_class, PhoshSearchResultContent, desc_lbl);

  gtk_widget_class_set_css_name (widget_class, "phosh-search-result-content");
}


static void
phosh_search_result_content_init (PhoshSearchResultContent *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


/**
 * phosh_search_result_content_new:
 * @result: (transfer none): the #PhoshSearchResult to show
 *
 * Create a #PhoshSearchResultContent to represent a #PhoshSearchResult
 *
 * Returns: (transfer float): a new #PhoshSearchResultContent
 */
GtkWidget *
phosh_search_result_content_new (PhoshSearchResult *result)
{
  return g_object_new (PHOSH_TYPE_SEARCH_RESULT_CONTENT,
                       "result", result,
                       NULL);
}


/**
 * phosh_search_result_content_get_result:
 *
 * Get the #PhoshSearchResult currently in @self
 *
 * Returns: (transfer none): the current #PhoshSearchResult in @self
 */
PhoshSearchResult *
phosh_search_result_content_get_result (PhoshSearchResultContent *self)
{
  g_return_val_if_fail (PHOSH_IS_SEARCH_RESULT_CONTENT (self), NULL);

  return self->result;
}

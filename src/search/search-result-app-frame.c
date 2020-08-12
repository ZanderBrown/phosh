/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-search-result-app-frame"

#include "config.h"

#include "search-result-app-frame.h"
#include "search-result-source.h"
#include "app-grid-button.h"


/**
 * SECTION:search-result-app-frame
 * @short_description: A frame containing one or more #PhoshAppGridButton
 * @Title: PhoshSearchResultAppFrame
 *
 * An alternative to #PhoshSearchResultFrame using #PhoshAppGridButtons
 * generated from the result #PhoshSearchResult:id
 */


struct _PhoshSearchResultAppFrame {
  GtkListBoxRow            parent;

  PhoshSearchResultSource *source;

  GtkFlowBox              *apps;
};
typedef struct _PhoshSearchResultAppFrame PhoshSearchResultAppFrame;


G_DEFINE_TYPE (PhoshSearchResultAppFrame, phosh_search_result_app_frame, GTK_TYPE_LIST_BOX_ROW)


static void
phosh_search_result_app_frame_dispose (GObject *object)
{
  PhoshSearchResultAppFrame *self = PHOSH_SEARCH_RESULT_APP_FRAME (object);

  // Don't clear bindings, they are unref'd automatically

  g_clear_object (&self->source);

  G_OBJECT_CLASS (phosh_search_result_app_frame_parent_class)->dispose (object);
}


static void
item_activated (GtkFlowBox                *box,
                GtkFlowBoxChild           *row,
                PhoshSearchResultAppFrame *self)
{
  g_return_if_fail (PHOSH_IS_SEARCH_RESULT_APP_FRAME (self));

  /* TODO */
}


static void
phosh_search_result_app_frame_class_init (PhoshSearchResultAppFrameClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_search_result_app_frame_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/search-result-app-frame.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshSearchResultAppFrame, apps);

  gtk_widget_class_bind_template_callback (widget_class, item_activated);

  gtk_widget_class_set_css_name (widget_class, "phosh-search-app-result-frame");
}


static void
phosh_search_result_app_frame_init (PhoshSearchResultAppFrame *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


/**
 * phosh_search_result_app_frame_new:
 *
 * Create a #PhoshSearchResultAppFrame, use
 * phosh_search_result_app_frame_bind_source() to populate it
 *
 * Returns: (transfer float): a new #PhoshSearchResultAppFrame
 */
GtkWidget *
phosh_search_result_app_frame_new (void)
{
  return g_object_new (PHOSH_TYPE_SEARCH_RESULT_APP_FRAME, NULL);
}


static GtkWidget *
create_row (gpointer item, gpointer data)
{
  PhoshSearchResult *result = item;
  GDesktopAppInfo *info = NULL;
  const char *desktop_id = NULL;

  desktop_id = phosh_search_result_get_id (result);
  info = g_desktop_app_info_new (desktop_id);

  if (G_UNLIKELY (info == NULL)) {
    g_warning ("Unknown desktop-id: “%s”", desktop_id);

    return NULL;
  }

  return phosh_app_grid_button_new (G_APP_INFO (info));
}


/**
 * phosh_search_result_app_frame_bind_source:
 * @self: the #PhoshSearchResultAppFrame
 * @source: the #PhoshSearchResultSource to bind to
 *
 * Bind @self to a #PhoshSearchResultSource, note @self can be reused for
 * different #PhoshSearchResultSources
 */
void
phosh_search_result_app_frame_bind_source (PhoshSearchResultAppFrame *self,
                                           PhoshSearchResultSource   *source)
{
  g_return_if_fail (PHOSH_IS_SEARCH_RESULT_APP_FRAME (self));
  g_return_if_fail (PHOSH_IS_SEARCH_RESULT_SOURCE (source));

  g_set_object (&self->source, source);

  gtk_flow_box_bind_model (GTK_FLOW_BOX (self->apps),
                           G_LIST_MODEL (source),
                           create_row,
                           self,
                           NULL);
}

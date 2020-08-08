/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-search-result-source"

#include "config.h"
#include "search-result-source.h"
#include "search-result.h"
#include "search-source.h"

#include <gio/gio.h>

/**
 * SECTION:search-result-source
 * @short_description: A #GListModel containing one or more #PhoshSearchResu;t
 * @Title: PhoshSearchResultSource
 *
 *
 */


enum {
  PROP_0,
  PROP_SOURCE,
  PROP_ID,
  PROP_APP_INFO,
  PROP_TOTAL_RESULTS,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


typedef struct _PhoshSearchResultSource {
  GObject            parent;

  PhoshSearchSource *source;
  guint              total;
  GListStore        *list;
} PhoshSearchResultSource;


static void list_iface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshSearchResultSource, phosh_search_result_source, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_iface_init))


static void
phosh_search_result_source_dispose (GObject *object)
{
  PhoshSearchResultSource *self = PHOSH_SEARCH_RESULT_SOURCE (object);

  g_clear_pointer (&self->source, phosh_search_source_unref);
  g_clear_object (&self->list);

  G_OBJECT_CLASS (phosh_search_result_source_parent_class)->dispose (object);
}


static void
phosh_search_result_source_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  PhoshSearchResultSource *self = PHOSH_SEARCH_RESULT_SOURCE (object);

  switch (property_id) {
    case PROP_SOURCE:
      self->source = g_value_dup_boxed (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_search_result_source_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  PhoshSearchResultSource *self = PHOSH_SEARCH_RESULT_SOURCE (object);

  switch (property_id) {
    case PROP_SOURCE:
      g_value_set_boxed (value, phosh_search_result_source_get_source (self));
      break;
    case PROP_ID:
      g_value_set_string (value,
                          phosh_search_result_source_get_id (self));
      break;
    case PROP_APP_INFO:
      g_value_set_object (value,
                          phosh_search_result_source_get_app_info (self));
      break;
    case PROP_TOTAL_RESULTS:
      g_value_set_uint (value,
                        phosh_search_result_source_get_total_results (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_search_result_source_class_init (PhoshSearchResultSourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = phosh_search_result_source_dispose;
  object_class->set_property = phosh_search_result_source_set_property;
  object_class->get_property = phosh_search_result_source_get_property;


  /**
   * PhoshSearchResultSource:source:
   *
   * The #PhoshSearchSource wrapped by #PhoshSearchResultSource:id and
   * #PhoshSearchResultSource:app-info
   */
  pspecs[PROP_SOURCE] =
    g_param_spec_boxed ("source", "Result source", "Underlying source data",
                        PHOSH_TYPE_SEARCH_SOURCE,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  /**
   * PhoshSearchResultSource:id:
   *
   * See phosh_search_source_get_id()
   */
  pspecs[PROP_ID] =
    g_param_spec_string ("id", "ID", "Source id",
                         NULL,
                         G_PARAM_READABLE);

  /**
   * PhoshSearchResultSource:app-info:
   *
   * See phosh_search_source_get_app_info()
   */
  pspecs[PROP_APP_INFO] =
    g_param_spec_object ("app-info", "App Info", "Source information",
                         G_TYPE_APP_INFO,
                         G_PARAM_READABLE);

  /**
   * PhoshSearchResultSource:total-results:
   *
   * The total number of results from the source, this may be far greater
   * than the value returned by g_list_model_get_n_items() and should be used
   * to calculate "n more" results
   */
  pspecs[PROP_TOTAL_RESULTS] =
    g_param_spec_uint ("total-results",
                       "Total Results",
                       "Total number of results",
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static GType
list_get_item_type (GListModel *list)
{
  return PHOSH_TYPE_SEARCH_RESULT;
}


static gpointer
list_get_item (GListModel *list, guint position)
{
  PhoshSearchResultSource *self = PHOSH_SEARCH_RESULT_SOURCE (list);

  return g_list_model_get_item (G_LIST_MODEL (self->list), position);
}


static unsigned int
list_get_n_items (GListModel *list)
{
  PhoshSearchResultSource *self = PHOSH_SEARCH_RESULT_SOURCE (list);

  return g_list_model_get_n_items (G_LIST_MODEL (self->list));
}


static void
list_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = list_get_item_type;
  iface->get_item = list_get_item;
  iface->get_n_items = list_get_n_items;
}


static void
items_changed (GListModel              *list,
               guint                    position,
               guint                    removed,
               guint                    added,
               PhoshSearchResultSource *self)
{
  g_return_if_fail (PHOSH_IS_SEARCH_RESULT_SOURCE (self));

  g_list_model_items_changed (G_LIST_MODEL (self), position, removed, added);
}


static void
phosh_search_result_source_init (PhoshSearchResultSource *self)
{
  self->list = g_list_store_new (PHOSH_TYPE_SEARCH_RESULT);

  g_signal_connect (self->list,
                    "items-changed",
                    G_CALLBACK (items_changed),
                    self);
}


/**
 * phosh_search_result_source_new:
 * @source: the underlying #PhoshSearchSource
 *
 * Creat a #PhoshSearchResultSource for results from a #PhoshSearchSource
 *
 * Returns: (transfer full): a new #PhoshSearchResultSource
 */
PhoshSearchResultSource *
phosh_search_result_source_new (PhoshSearchSource *source)
{
  return g_object_new (PHOSH_TYPE_SEARCH_RESULT_SOURCE,
                       "source", source,
                       NULL);
}


/**
 * phosh_search_result_source_add:
 * @self: the #PhoshSearchResultSource
 * @result: (transfer none): the #PhoshSearchResult to add
 *
 * Add @result to the list of results in @self
 */
void
phosh_search_result_source_add (PhoshSearchResultSource *self,
                                PhoshSearchResult       *result)
{
  g_return_if_fail (PHOSH_IS_SEARCH_RESULT_SOURCE (self));

  g_list_store_append (self->list, result);
}


/**
 * phosh_search_result_source_get_source:
 * @self: the #PhoshSearchResultSource
 *
 * Get the #PhoshSearchSource providing data for @self
 *
 * Returns: (transfer none): the underlying #PhoshSearchSource
 */
PhoshSearchSource *
phosh_search_result_source_get_source (PhoshSearchResultSource *self)
{
  g_return_val_if_fail (PHOSH_IS_SEARCH_RESULT_SOURCE (self), NULL);

  return self->source;
}


/**
 * phosh_search_result_source_get_id:
 * @self: the #PhoshSearchResultSource
 *
 * See phosh_search_source_get_id()
 *
 * Returns: (transfer none): the source id
 */
const char *
phosh_search_result_source_get_id (PhoshSearchResultSource *self)
{
  g_return_val_if_fail (PHOSH_IS_SEARCH_RESULT_SOURCE (self), NULL);

  return phosh_search_source_get_id (self->source);
}


/**
 * phosh_search_result_source_get_app_info:
 * @self: the #PhoshSearchResultSource
 *
 * See phosh_search_source_get_app_info()
 *
 * Returns: (transfer none): the source #GAppInfo
 */
GAppInfo *
phosh_search_result_source_get_app_info (PhoshSearchResultSource *self)
{
  g_return_val_if_fail (PHOSH_IS_SEARCH_RESULT_SOURCE (self), NULL);

  return phosh_search_source_get_app_info (self->source);
}


/**
 * phosh_search_result_source_get_total_results:
 * @self: the #PhoshSearchResultSource
 *
 * Get the total number of results the source has vs the number actually
 * stored in @self (as reported by g_list_model_get_n_items())
 *
 * Returns: the total number of results
 */
guint
phosh_search_result_source_get_total_results (PhoshSearchResultSource *self)
{
  g_return_val_if_fail (PHOSH_IS_SEARCH_RESULT_SOURCE (self), 0);

  return self->total;
}

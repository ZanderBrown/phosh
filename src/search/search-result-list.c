/*
 * Copyright © 2020 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-search-result-list"

#include "search-result-list.h"
#include "search-result-source.h"

#include <gio/gio.h>

/**
 * SECTION:search-result-list
 * @short_description: A #GListModel containing one or
 *                     more #PhoshSearchResultSource
 * @Title: PhoshSearchResultList
 *
 * A list of #PhoshSearchSource containing #PhoshSearchResults
 */


enum {
  SIGNAL_EMPTY,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


typedef struct _PhoshSearchResultList {
  GObject     parent;

  GListStore *list;
} PhoshSearchResultList;


static void list_iface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshSearchResultList, phosh_search_result_list, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_iface_init))


static void
phosh_search_result_list_dispose (GObject *object)
{
  PhoshSearchResultList *self = PHOSH_SEARCH_RESULT_LIST (object);

  g_clear_object (&self->list);

  G_OBJECT_CLASS (phosh_search_result_list_parent_class)->dispose (object);
}


static void
phosh_search_result_list_class_init (PhoshSearchResultListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = phosh_search_result_list_dispose;

  /**
   * PhoshSearchResultList::empty:
   * @self: the #PhoshSearchResultList
   *
   * The last item has been removed, @self is not empty
   */
  signals[SIGNAL_EMPTY] = g_signal_new ("empty",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0,
                                        NULL,
                                        NULL,
                                        NULL,
                                        G_TYPE_NONE,
                                        0);
}


static GType
list_get_item_type (GListModel *list)
{
  return PHOSH_TYPE_SEARCH_RESULT_SOURCE;
}


static gpointer
list_get_item (GListModel *list, guint position)
{
  PhoshSearchResultList *self = PHOSH_SEARCH_RESULT_LIST (list);

  return g_list_model_get_item (G_LIST_MODEL (self->list), position);
}


static unsigned int
list_get_n_items (GListModel *list)
{
  PhoshSearchResultList *self = PHOSH_SEARCH_RESULT_LIST (list);

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
items_changed (GListModel            *list,
               guint                  position,
               guint                  removed,
               guint                  added,
               PhoshSearchResultList *self)
{
  g_autoptr (PhoshSearchResultSource) item = NULL;

  g_return_if_fail (PHOSH_IS_SEARCH_RESULT_LIST (self));

  g_list_model_items_changed (G_LIST_MODEL (self), position, removed, added);

  item = g_list_model_get_item (G_LIST_MODEL (self->list), 0);

  if (item == NULL) {
    g_signal_emit (self, signals[SIGNAL_EMPTY], 0);
  }
}


static void
phosh_search_result_list_init (PhoshSearchResultList *self)
{
  self->list = g_list_store_new (PHOSH_TYPE_SEARCH_RESULT_SOURCE);

  g_signal_connect (self->list,
                    "items-changed",
                    G_CALLBACK (items_changed),
                    self);
}


/**
 * phosh_search_result_list_new:
 *
 * Creates a new #PhoshSearchResultList that will hold various
 * #PhoshSearchResultSources
 *
 * Returns: (transfer full): a new #PhoshSearchResultList
 */
PhoshSearchResultList *
phosh_search_result_list_new (void)
{
  return g_object_new (PHOSH_TYPE_SEARCH_RESULT_LIST, NULL);
}


/**
 * phosh_search_result_list_add:
 * @self: the #PhoshSearchResultList
 * @source: (transfer none): the #PhoshSearchResultSource to add to @self
 *
 * Adds a #PhoshSearchResultSource to the list in @self
 */
void
phosh_search_result_list_add (PhoshSearchResultList   *self,
                              PhoshSearchResultSource *source)
{
  g_return_if_fail (PHOSH_IS_SEARCH_RESULT_LIST (self));

  g_list_store_append (self->list, source);
}

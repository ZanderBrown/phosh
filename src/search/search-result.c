/*
 * Copyright © 2019-2020 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3-or-later
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#define G_LOG_DOMAIN "phosh-search-result"

#include "search-result.h"
#include "search-result-meta.h"


/**
 * SECTION:search-result
 * @short_description: Information about a search result
 * @Title: PhoshSearchResult
 *
 * A #GObject wrapper around #PhoshSearchResultMeta suitable for use in a
 * #GListModel
 */


enum {
  PROP_0,
  PROP_DATA,
  PROP_ID,
  PROP_TITLE,
  PROP_DESCRIPTION,
  PROP_ICON,
  PROP_CLIPBOARD_TEXT,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


struct _PhoshSearchResult {
  GObject                parent;

  PhoshSearchResultMeta *data;
};


G_DEFINE_TYPE (PhoshSearchResult, phosh_search_result, G_TYPE_OBJECT)


static void
phosh_search_result_dispose (GObject *object)
{
  PhoshSearchResult *self = PHOSH_SEARCH_RESULT (object);

  g_clear_pointer (&self->data, phosh_search_result_meta_unref);

  G_OBJECT_CLASS (phosh_search_result_parent_class)->dispose (object);
}


static void
phosh_search_result_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  PhoshSearchResult *self = PHOSH_SEARCH_RESULT (object);

  switch (property_id) {
    case PROP_DATA:
      self->data = g_value_dup_boxed (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_search_result_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  PhoshSearchResult *self = PHOSH_SEARCH_RESULT (object);

  switch (property_id) {
    case PROP_DATA:
      g_value_set_boxed (value, self->data);
      break;
    case PROP_ID:
      g_value_set_string (value,
                          phosh_search_result_meta_get_id (self->data));
      break;
    case PROP_TITLE:
      g_value_set_string (value,
                          phosh_search_result_meta_get_title (self->data));
      break;
    case PROP_DESCRIPTION:
      g_value_set_string (value,
                          phosh_search_result_meta_get_description (self->data));
      break;
    case PROP_ICON:
      g_value_set_object (value,
                          phosh_search_result_meta_get_icon (self->data));
      break;
    case PROP_CLIPBOARD_TEXT:
      g_value_set_string (value,
                          phosh_search_result_meta_get_clipboard_text (self->data));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
phosh_search_result_class_init (PhoshSearchResultClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = phosh_search_result_dispose;
  object_class->set_property = phosh_search_result_set_property;
  object_class->get_property = phosh_search_result_get_property;

  /**
   * PhoshSearchResult::data:
   *
   * The underlying #PhoshSearchResultMeta
   */
  pspecs[PROP_DATA] =
    g_param_spec_boxed ("data", "Result data", "Underlying result metadata",
                        PHOSH_TYPE_SEARCH_RESULT_META,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  /**
   * PhoshSearchResult::id:
   *
   * See phosh_search_result_meta_get_id()
   */
  pspecs[PROP_ID] =
    g_param_spec_string ("id", "ID", "Result id",
                         NULL,
                         G_PARAM_READABLE);

  /**
   * PhoshSearchResult::title:
   *
   * See phosh_search_result_meta_get_title()
   */
  pspecs[PROP_TITLE] =
    g_param_spec_string ("title", "Title", "Result title",
                         NULL,
                         G_PARAM_READABLE);

  /**
   * PhoshSearchResult::description:
   *
   * See phosh_search_result_meta_get_description()
   */
  pspecs[PROP_DESCRIPTION] =
    g_param_spec_string ("description", "Description", "Result description",
                         NULL,
                         G_PARAM_READABLE);

  /**
   * PhoshSearchResult::icon:
   *
   * See phosh_search_result_meta_get_description()
   */
  pspecs[PROP_ICON] =
    g_param_spec_object ("icon", "Icon", "Result icon",
                         G_TYPE_ICON,
                         G_PARAM_READABLE);

  /**
   * PhoshSearchResult::clipboard-text:
   *
   * See phosh_search_result_meta_get_clipboard_text()
   */
  pspecs[PROP_CLIPBOARD_TEXT] =
    g_param_spec_string ("clipboard-text", "Clipboard Text", "Text to copy to clipboard",
                         NULL,
                         G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
phosh_search_result_init (PhoshSearchResult *self)
{
}


/**
 * phosh_search_result_new:
 * @data: the #PhoshSearchResultMeta to wrap
 *
 * Create a new #PhoshSearchResult that wraps @data
 *
 * Returns: (transfer full): the new #PhoshSearchResult
 */
PhoshSearchResult *
phosh_search_result_new (PhoshSearchResultMeta *data)
{
  return g_object_new (PHOSH_TYPE_SEARCH_RESULT,
                       "data", data,
                       NULL);
}


/**
 * phosh_search_result_get_id:
 * @self: the #PhoshSearchResult
 *
 * See phosh_search_result_meta_get_id()
 *
 * Returns: (transfer none): the result "id" of @self
 */
const char *
phosh_search_result_get_id (PhoshSearchResult *self)
{
  g_return_val_if_fail (PHOSH_IS_SEARCH_RESULT (self), NULL);

  return phosh_search_result_meta_get_id (self->data);
}


/**
 * phosh_search_result_get_title:
 * @self: the #PhoshSearchResult
 *
 * See phosh_search_result_meta_get_title()
 *
 * Returns: (transfer none): the title of @self
 */
const char *
phosh_search_result_get_title (PhoshSearchResult *self)
{
  g_return_val_if_fail (PHOSH_IS_SEARCH_RESULT (self), NULL);

  return phosh_search_result_meta_get_title (self->data);
}


/**
 * phosh_search_result_get_description:
 * @self: the #PhoshSearchResult
 *
 * See phosh_search_result_get_description()
 *
 * Returns: (transfer none) (nullable): the description of @self
 */
const char *
phosh_search_result_get_description (PhoshSearchResult *self)
{
  g_return_val_if_fail (PHOSH_IS_SEARCH_RESULT (self), NULL);

  return phosh_search_result_meta_get_description (self->data);
}


/**
 * phosh_search_result_get_clipboard_text:
 * @self: the #PhoshSearchResult
 *
 * See phosh_search_result_meta_get_clipboard_text()
 *
 * Returns: (transfer none) (nullable): the clipboard text of @self
 */
const char *
phosh_search_result_get_clipboard_text (PhoshSearchResult *self)
{
  g_return_val_if_fail (PHOSH_IS_SEARCH_RESULT (self), NULL);

  return phosh_search_result_meta_get_clipboard_text (self->data);
}


/**
 * phosh_search_result_get_icon:
 * @self: the #PhoshSearchResult
 *
 * See phosh_search_result_get_icon()
 *
 * Returns: (transfer none) (nullable): the icon of @self
 */
GIcon *
phosh_search_result_get_icon (PhoshSearchResult *self)
{
  g_return_val_if_fail (PHOSH_IS_SEARCH_RESULT (self), NULL);

  return phosh_search_result_meta_get_icon (self->data);
}

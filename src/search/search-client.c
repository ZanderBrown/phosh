/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include "search-client.h"
#include "search-result-meta.h"
#include "search-source.h"
#include "phosh-searchd.h"

enum State {
  CREATED,
  STARTING,
  READY,
};

typedef struct _PhoshSearchClientPrivate PhoshSearchClientPrivate;
struct _PhoshSearchClientPrivate {
  PhoshSearchDBusSearch *server;

  enum State             state;

  GRegex                *highlight;
  GRegex                *splitter;

  GCancellable          *cancellable;
};

static void async_iface_init (GAsyncInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (PhoshSearchClient, phosh_search_client, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (PhoshSearchClient)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, async_iface_init))


enum {
  SOURCE_RESULTS_CHANGED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };


static void
phosh_search_client_finalize (GObject *object)
{
  PhoshSearchClient *self = PHOSH_SEARCH_CLIENT (object);
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);

  g_cancellable_cancel (priv->cancellable);

  g_clear_object (&priv->server);
  g_clear_object (&priv->cancellable);

  g_clear_pointer (&priv->highlight, g_regex_unref);
  g_clear_pointer (&priv->splitter, g_regex_unref);

  G_OBJECT_CLASS (phosh_search_client_parent_class)->finalize (object);
}


static void
phosh_search_client_class_init (PhoshSearchClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phosh_search_client_finalize;

  signals[SOURCE_RESULTS_CHANGED] = g_signal_new ("source-results-changed",
                                                  G_TYPE_FROM_CLASS (klass),
                                                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                                                  0,
                                                  NULL, NULL, NULL,
                                                  G_TYPE_NONE,
                                                  2,
                                                  G_TYPE_STRING,
                                                  G_TYPE_PTR_ARRAY);
}


static void
results (PhoshSearchDBusSearch *server,
         const char            *source_id,
         GVariant              *variant,
         PhoshSearchClient     *self)
{
  GVariantIter iter;
  GVariant *item;
  GPtrArray *results = NULL;

  results = g_ptr_array_new_with_free_func ((GDestroyNotify) phosh_search_result_meta_unref);

  g_variant_iter_init (&iter, variant);
  while ((item = g_variant_iter_next_value (&iter))) {
    g_autoptr (PhoshSearchResultMeta) result = NULL;

    result = phosh_search_result_meta_deserialise (item);

    g_ptr_array_add (results, phosh_search_result_meta_ref (result));

    g_clear_pointer (&item, g_variant_unref);
  }

  g_signal_emit (self,
                 signals[SOURCE_RESULTS_CHANGED],
                 g_quark_from_string (source_id),
                 source_id,
                 results);
}


struct InitData {
  PhoshSearchClient *self;
  GTask             *task;
};

static void
got_search (GObject       *source,
            GAsyncResult  *result,
            gpointer       user_data)
{
  g_autoptr (GError) error = NULL;
  g_autofree struct InitData *data = user_data;
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (data->self);

  priv->server = phosh_search_dbus_search_proxy_new_finish (result, &error);

  priv->state = READY;

  if (error) {
    g_task_return_error (data->task, error);
    return;
  }

  g_signal_connect (priv->server,
                    "source-results-changed",
                    G_CALLBACK (results),
                    data->self);

  g_task_return_boolean (data->task, TRUE);
}


static void
phosh_search_client_init_async (GAsyncInitable      *initable,
                                int                  io_priority,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
  PhoshSearchClient *self = PHOSH_SEARCH_CLIENT (initable);
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  GTask* task = NULL;
  struct InitData *data;

  switch (priv->state) {
      case CREATED:
        priv->state = STARTING;

        task = g_task_new (initable, cancellable, callback, user_data);

        data = g_new0 (struct InitData, 1);
        data->self = self;
        data->task = task;

        phosh_search_dbus_search_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                                    G_DBUS_PROXY_FLAGS_NONE,
                                                    "sm.puri.Phosh.Search",
                                                    "/sm/puri/Phosh/Search",
                                                    cancellable,
                                                    got_search,
                                                    data);
        break;
      case STARTING:
        g_critical ("Already initialising");
        break;
      case READY:
        g_critical ("Already initialised");
        break;
      default:
        g_assert_not_reached ();
    }
}


static gboolean
phosh_search_client_init_finish (GAsyncInitable  *initable,
                                 GAsyncResult    *result,
                                 GError         **error)
{
  g_return_val_if_fail (g_task_is_valid (result, initable), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}


static void
async_iface_init (GAsyncInitableIface *iface)
{
  iface->init_async = phosh_search_client_init_async;
  iface->init_finish = phosh_search_client_init_finish;
}


static void
phosh_search_client_init (PhoshSearchClient *self)
{
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  g_autoptr (GError) error = NULL;

  priv->highlight = NULL;
  priv->splitter = g_regex_new ("\\s+",
                                G_REGEX_CASELESS | G_REGEX_MULTILINE,
                                0,
                                &error);
  priv->state = CREATED;
  priv->cancellable = g_cancellable_new ();

  if (error) {
    g_error ("Bad Regex: %s", error->message);
  }
}


static void
got_query (GObject      *source,
           GAsyncResult *result,
           gpointer      user_data)
{
  PhoshSearchClient *self = PHOSH_SEARCH_CLIENT (user_data);
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  g_autoptr (GError) error = NULL;

  phosh_search_dbus_search_call_query_finish (priv->server,
                                              NULL,
                                              result,
                                              &error);

  if (error) {
    g_critical ("Unable to send search: %s", error->message);
  }
}


void
phosh_search_client_query (PhoshSearchClient *self,
                           const char        *str)
{
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  g_autofree char *regex_terms = NULL;
  g_autofree char *regex = NULL;
  g_autofree char *striped = NULL;
  g_auto (GStrv) parts = NULL;
  g_auto (GStrv) escaped = NULL;
  g_autoptr (GError) error = NULL;
  int len = 0;
  int i = 0;

  phosh_search_dbus_search_call_query (priv->server,
                                       str,
                                       priv->cancellable,
                                       got_query,
                                       self);

  striped = g_strstrip (g_strdup (str));
  parts = g_regex_split (priv->splitter, striped, 0);

  len = parts ? g_strv_length (parts) : 0;

  escaped = g_new0 (char *, len + 1);

  while (parts[i]) {
    escaped[i] = g_regex_escape_string (parts[i], -1);

    i++;
  }
  escaped[len] = NULL;

  regex_terms = g_strjoinv ("|", escaped);
  regex = g_strconcat ("(", regex_terms, ")", NULL);
  priv->highlight = g_regex_new (regex,
                                 G_REGEX_CASELESS | G_REGEX_MULTILINE,
                                 0,
                                 &error);

  if (error) {
    g_warning ("Unable to prepare highlighter: %s", error->message);
    g_clear_error (&error);
    priv->highlight = NULL;
  }
}


const char *
phosh_search_client_markup_string (PhoshSearchClient *self,
                                   const char        *string)
{
  PhoshSearchClientPrivate *priv = phosh_search_client_get_instance_private (self);
  g_autoptr (GError) error = NULL;
  char *marked = NULL;

  if (string == NULL) {
    return NULL;
  }

  marked = g_regex_replace (priv->highlight,
                            string, -1, 0,
                            "<b>\\1</b>",
                            0,
                            &error);

  if (error) {
    return g_strdup (string);
  }

  return marked;
}

void
phosh_search_client_new (GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
  g_async_initable_new_async (PHOSH_TYPE_SEARCH_CLIENT,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              NULL);
}


PhoshSearchClient *
phosh_search_client_new_finish (GObject       *source,
                                GAsyncResult  *result,
                                GError       **error)
{
  return PHOSH_SEARCH_CLIENT (g_async_initable_new_finish (G_ASYNC_INITABLE (source),
                                                           result,
                                                           error));
}

/*
 * Copyright (C) 2019 Zander Brown
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * Author: Zander Brown <zbrown@gnome.org>
 */

#include <search-client.h>
#include <search-result-meta.h>
#include <phosh-searchd.h>


static void
results (PhoshSearchClient *client,
         const char        *source_id,
         GPtrArray         *results)
{
  g_print ("%s\n", source_id);

  for (int i = 0; i < results->len; i++) {
    PhoshSearchResult *result = g_ptr_array_index (results, i);

    g_print (" - %s\n", phosh_search_result_meta_get_id (result));
    g_print ("   name: %s\n", phosh_search_client_markup_string (client, phosh_search_result_meta_get_title (result)));
    g_print ("   desc: %s\n", phosh_search_client_markup_string (client, phosh_search_result_meta_get_description (result)));
    if (phosh_search_result_meta_get_icon (result)) {
      g_print ("   gi-t: %s\n", G_OBJECT_TYPE_NAME (phosh_search_result_meta_get_icon (result)));
    }
    if (phosh_search_result_meta_get_clipboard_text (result)) {
      g_print ("   cp-t: %s\n", phosh_search_result_meta_get_clipboard_text (result));
    }
  }

  g_print ("\n");
}


static void
got_client (GObject       *source,
            GAsyncResult  *result,
            gpointer       user_data)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (PhoshSearchClient) client = NULL;

  client = phosh_search_client_new_finish (source, result, &error);

  g_signal_connect (client, "source-results-changed", G_CALLBACK (results), NULL);

  phosh_search_client_query (client, "test");
}


int
main (int argc, char**argv)
{
  g_autoptr (GMainLoop) loop = NULL;
  g_autoptr (PhoshSearchDBusSearch) search = NULL;

  phosh_search_client_new (NULL, got_client, NULL);

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

  return 0;
}

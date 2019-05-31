/*
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0+
 */

#define G_LOG_DOMIAN "phosh-search-provider"

#include "search-provider.h"

#define GROUP_NAME "Shell Search Provider"

void
find_them (void)
{
  const char *const *data_dirs = g_get_system_data_dirs ();
  const char *data_dir = NULL;
  int i = 0;

  while ((data_dir = data_dirs[i])) {
    g_autofree char *dir = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GDir) list = NULL;
    const char* name = NULL;

    i++;

    dir = g_build_filename (data_dir, "gnome-shell", "search-providers", NULL);
    list = g_dir_open (dir, 0, &error);

    g_message ("look in %s", dir);

    if (error) {
      if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
        g_warning ("Can't look for in %s: %s", dir, error->message);
      }
      g_clear_error (&error);
      continue;
    }

    while ((name = g_dir_read_name (list))) {
      g_autofree char *provider = NULL;
      g_autoptr (GKeyFile) data = NULL;
      int version = 0;

      provider = g_build_filename (dir, name, NULL);
      data = g_key_file_new ();

      g_key_file_load_from_file (data, provider, G_KEY_FILE_NONE, &error);

      if (error) {
        g_warning ("Can't read %s: %s", provider, error->message);
        g_clear_error (&error);
        continue;
      }

      if (!g_key_file_has_group (data, GROUP_NAME)) {
        g_warning ("%s doesn't define a search provider", provider);
        continue;
      }

      version = g_key_file_get_integer (data, GROUP_NAME, "Version", &error);

      if (error) {
        g_warning ("Failed to fetch provider version %s: %s", provider, error->message);
        g_clear_error (&error);
        continue;
      }

      if (version != 2) {
        g_warning ("Provider %s implements version %i but we only support version 2", provider, version);
        continue;
      }

      g_message ("We got one! %s", provider);
    }
  }
}
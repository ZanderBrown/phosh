/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "overview.h"

static void
test_phosh_overview_new(void)
{
  GtkWidget *window = phosh_overview_new ();
  g_assert (window);
  gtk_widget_destroy (window);
}


gint
main (gint argc,
      gchar *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func("/phosh/overview/new", test_phosh_overview_new);
  return g_test_run();
}

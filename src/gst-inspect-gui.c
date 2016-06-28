/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gst/gst.h>

#include <gst/editor/editor.h>
#include <gst/element-browser/browser.h>
#include <gst/debug-ui/debug-ui.h>

int
main (int argc, char * argv[])
{
  GstElementFactory * chosen = NULL;

  // const struct poptOption *gst_table;
  GOptionContext * ctx;

  // gst_table = gst_init_get_popt_table ();
  ctx = g_option_context_new (PACKAGE);
  // g_option_context_add_main_entries (ctx, options, GETTEXT_PACKAGE);
  g_option_context_add_group (ctx, gst_init_get_option_group ());

  gst_init (&argc, &argv);
  gtk_init (&argc, &argv);

  gste_init ();

  chosen = (GstElementFactory *)gst_element_browser_pick_modal ();
  if (chosen)
    g_print ("selected '%s'\n", GST_OBJECT_NAME (chosen));
  else
    g_print ("didn't choose any\n");
  return 0;
}

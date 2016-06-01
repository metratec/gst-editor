/* gst-editor
 * Copyright (C) <2001> Steve Baker <stevebaker_org@yahoo.co.uk>
 * Copyright (C) <2002, 2003> Andy Wingo <wingo at pobox dot com>
 * Copyright (C) <2004> Jan Schmidt <thaytan@mad.scientist.com>
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
 * 
 * Function to initialise data in the gste-common library
 */

#include <glib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gste-common-priv.h"

static gboolean gste_done_init = FALSE;

void
gste_init (void)
{
  if (gste_done_init)
    return;

  gste_debug_init ();

  gste_done_init = TRUE;
}

/* given the name of the glade file, return the newly allocated full path to it
 * if it exists and NULL if not. */
gchar *
gste_get_ui_file (const char *filename)
{
  char *path;

  /* looking for glade file in uninstalled dir */
  path = g_build_filename (GLADEUI_UNINSTALLED_DIR, filename, NULL);
  if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
    return path;
  g_free (path);

  /* looking for glade file in installed dir */
  path = g_build_filename (GST_EDITOR_DATA_DIR, filename, NULL);
  if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
    return path;
  g_free (path);

  /* not found */
  return NULL;
}

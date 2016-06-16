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
#include "config.h"

#include <gtk/gtk.h>

#include <pixmaps/pixmaps.h>

#include "namedicons.h"

/* add Icons with given size */
static void
add_sized (const guchar * inline_data,
    GtkIconSize size, const gchar * icon_name)
{
  GdkPixbuf *pixbuf;

  pixbuf = gdk_pixbuf_new_from_inline (-1, inline_data, FALSE, NULL);
  if (!pixbuf)
    return;

  /*
   * This way of registering named icons is deprecated in
   * Gtk v3.14 and replaced by the GResource mechanism
   * which however is only supported by GIO v2.32.
   * So we (have to) keep the inlined pixbufs as long as we're
   * targetting Gtk v3.10-v3.13.
   */
  gtk_icon_theme_add_builtin_icon (icon_name, size, pixbuf);

  g_object_unref (G_OBJECT (pixbuf));
}

/* init our stock items */
void
gst_editor_named_icons_init (void)
{
  static gboolean initialized = FALSE;

  if (initialized)
    return;

  initialized = TRUE;

  add_sized (gst_editor_stock_image, GTK_ICON_SIZE_DIALOG,
      GST_EDITOR_NAMED_ICON_LOGO);
}

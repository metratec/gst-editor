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

#include "gsteditorpopup.h"

void
gst_editor_popup_warning (const gchar * message)
{
  GtkWidget *dialog;

  /* throw up a dialog box; FIXME: we don't have a parent */
  /* FIXME: maybe even fallback the GError and do error
   * handling higher up ? */

  dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_WARNING,
      GTK_BUTTONS_CLOSE, "%s", message);
  gtk_widget_show (dialog);
  g_signal_connect_swapped (dialog, "response",
      G_CALLBACK (gtk_widget_destroy), dialog);
}

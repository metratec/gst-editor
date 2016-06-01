/* GStreamer
 * Copyright (C) <2007> Samuel Vinson <samuelv at users sourceforge net>
 * Copyright (C) <2002-2005> Jorg Schuler <jcsjcs at users sourceforge net>
 * Part of the gtkpod project. URL: http://www.gtkpod.org/
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
    
#include <gtk/gtk.h>
    
#include "config.h"
    
#include "gsteditor.h"
#include "gsteditorstatusbar.h"
    
/* stuff for statusbar */ 
#define STATUSBAR_TIMEOUT 4200
static GtkWidget *gst_editor_statusbar = NULL;
static guint statusbar_timeout_id = 0;
static guint statusbar_timeout = STATUSBAR_TIMEOUT;
void 
gst_editor_statusbar_init (GstEditor * editor) 
{
  
//    gst_editor_statusbar = gtkpod_xml_get_widget (main_window_xml, "gtkpod_status");
      gst_editor_statusbar = glade_xml_get_widget (editor->xml, "status_bar");
  statusbar_timeout = STATUSBAR_TIMEOUT;
} static gint 

gst_editor_statusbar_clear (gpointer data) 
{
  if (gst_editor_statusbar)
     {
    gtk_statusbar_pop (GTK_STATUSBAR (gst_editor_statusbar), 1);
    }
  statusbar_timeout_id = 0;    /* indicate that timeout handler is
                                   clear (0 cannot be a handler id) */
  return FALSE;
}
static void 
gst_editor_statusbar_reset_timeout (void) 
{
  if (statusbar_timeout_id != 0)       /* remove last timeout, if still present */
    gtk_timeout_remove (statusbar_timeout_id);
  statusbar_timeout_id = gtk_timeout_add (statusbar_timeout, 
      (GtkFunction) gst_editor_statusbar_clear, NULL);
}
void 
gst_editor_statusbar_timeout (guint timeout) 
{
  if (timeout == 0)
    statusbar_timeout = STATUSBAR_TIMEOUT;
  
  else
    statusbar_timeout = timeout;
  gst_editor_statusbar_reset_timeout ();
}
void 
gst_editor_statusbar_message (const gchar * message, ...) 
{
  if (gst_editor_statusbar)
     {
    va_list arg;
    gchar * text;
    guint context = 1;
    va_start (arg, message);
    text = g_strdup_vprintf (message, arg);
    va_end (arg);
    gtk_statusbar_pop (GTK_STATUSBAR (gst_editor_statusbar), context);
    gtk_statusbar_push (GTK_STATUSBAR (gst_editor_statusbar), context, text);
    gst_editor_statusbar_reset_timeout ();
    }
}



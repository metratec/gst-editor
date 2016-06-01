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


#ifndef __GST_EDITOR_H__
#define __GST_EDITOR_H__


#include <gst/gst.h>
#include <gst/editor/editor.h>
#include <glade/glade-xml.h>
#include <gtk/gtk.h>

#define GST_TYPE_EDITOR (gst_editor_get_type())
#define GST_EDITOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_EDITOR, GstEditor))
#define GST_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_EDITOR, GstEditorClass))
#define GST_IS_EDITOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_EDITOR))
#define GST_IS_EDITOR_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_EDITOR))
#define GST_EDITOR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_EDITOR, GstEditorClass))


#define GST_EDITOR_SET_OBJECT(item,object) (g_object_set_data (G_OBJECT (item), "gsteditorobject", (object)))
#define GST_EDITOR_GET_OBJECT(item) (g_object_get_data (G_OBJECT (item), "gsteditorobject"))


struct _GstEditor
{
  GObject object;

  GladeXML *xml;
  GtkWidget *window;
  GtkWidget *element_tree;
  GtkSpinButton *sw,*sh;
  

  gchar *filename;
  gboolean changed;
  gboolean need_name;

  GstEditorCanvas *canvas;

  GData *attributes;//one global attributes location all other point to
  GMutex *outputmutex;//only used for time measurement outputs
};

struct _GstEditorClass
{
  GObjectClass parent_class;
};

GType gst_editor_get_type (void);
GtkWidget *gst_editor_new (GstElement * element);
void gst_editor_load (GstEditor * editor, const gchar * file_name);
void gst_editor_on_spinbutton(GtkSpinButton * spinheight, GstEditor * editor);

#endif /* __GST_EDITOR_H__ */

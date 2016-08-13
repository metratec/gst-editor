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

#include <gtk/gtk.h>

#include <gst/editor/gsteditorcanvas.h>
#include <gst/common/gste-serialize.h>

#define GST_TYPE_EDITOR (gst_editor_get_type())
#define GST_EDITOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_EDITOR, GstEditor))
#define GST_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_EDITOR, GstEditorClass))
#define GST_IS_EDITOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_EDITOR))
#define GST_IS_EDITOR_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_EDITOR))
#define GST_EDITOR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_EDITOR, GstEditorClass))

typedef struct _GstEditor
{
  GObject object;

  GtkBuilder *builder;
  GtkWidget *save_dialog;
  GtkFileFilter *filter_gep, *filter_gsp;
  GtkWidget *window;
  GtkWidget *element_tree;
  GtkSpinButton *sw,*sh;
  GtkStatusbar *statusbar;
  guint statusbar_timeout_id;

  gchar *filename;
  gboolean changed;
  gboolean need_name;
  GsteSerializeFlags save_flags;

  GstEditorCanvas *canvas;

  GMutex outputmutex;//only used for time measurement outputs
} GstEditor;

typedef struct _GstEditorClass
{
  GObjectClass parent_class;
} GstEditorClass;

GType gst_editor_get_type (void);
GtkWidget *gst_editor_new (GstElement * element);
void gst_editor_load (GstEditor * editor, const gchar * file_name);

#endif /* __GST_EDITOR_H__ */

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
#ifndef __GST_EDITOR_CANVAS_H__
#define __GST_EDITOR_CANVAS_H__

#include <gst/gst.h>
#include <gtk/gtk.h>
#include <goocanvas.h>

#include <gst/editor/gsteditorproperty.h>
#include <gst/editor/gsteditoritem.h>
#include <gst/editor/gsteditorelement.h>
#include <gst/editor/gsteditorbin.h>

#define GST_TYPE_EDITOR_CANVAS (gst_editor_canvas_get_type())
#define GST_EDITOR_CANVAS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_EDITOR_CANVAS, GstEditorCanvas))
#define GST_EDITOR_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_EDITOR_CANVAS, GstEditorCanvasClass))
#define GST_IS_EDITOR_CANVAS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_EDITOR_CANVAS))
#define GST_IS_EDITOR_CANVAS_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_EDITOR_CANVAS))
#define GST_EDITOR_CANVAS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_EDITOR_CANVAS, GstEditorCanvasClass))

#define GST_EDITOR_CANVAS_ERROR \
    g_quark_from_static_string("gst-editor-canvas-error-quark")

enum GstEditorCanvasError {
  GST_EDITOR_CANVAS_ERROR_FAILED
};

typedef struct _GstEditorCanvas
{
  GooCanvas canvas;
  GstEditorBin *bin;
  GstEditorElement *selection;
  GstEditorProperty *property;
  GtkWidget *palette;
  gchar *status;
  GData *attributes;    /* list of name -> GstEditorItemAttr mappings */
  GRWLock globallock;
  gboolean autosize;
  gboolean live;
  gboolean show_all_bins;
  gdouble widthbackup, heightbackup;
} GstEditorCanvas;

typedef struct _GstEditorCanvasClass
{
   GooCanvasClass parent_class;
} GstEditorCanvasClass;

GType gst_editor_canvas_get_type (void);

static inline GstElement *
gst_editor_canvas_get_pipeline (GstEditorCanvas * canvas)
{
  return canvas->bin ? GST_ELEMENT_CAST (GST_EDITOR_ITEM (canvas->bin)->object)
                     : NULL;
}

gboolean gst_editor_canvas_load_with_metadata (GstEditorCanvas * canvas,
    GKeyFile * key_file, GError ** error);

GstElement * gst_editor_canvas_get_selected_bin (GstEditorCanvas * canvas, GError ** error);

#endif /* __GST_EDITOR_CANVAS_H__ */

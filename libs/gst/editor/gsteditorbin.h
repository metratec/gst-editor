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


#ifndef __GST_EDITOR_BIN_H__
#define __GST_EDITOR_BIN_H__


#include <gtk/gtk.h>
#include <gst/editor/gsteditor.h>
#include <gst/editor/gsteditorelement.h>


#define GST_TYPE_EDITOR_BIN (gst_editor_bin_get_type())
#define GST_EDITOR_BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_EDITOR_BIN, GstEditorBin))
#define GST_EDITOR_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_EDITOR_BIN, GstEditorBinClass))
#define GST_IS_EDITOR_BIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_EDITOR_BIN))
#define GST_IS_EDITOR_BIN_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_EDITOR_BIN))
#define GST_EDITOR_BIN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_EDITOR_BIN, GstEditorBinClass))


struct _GstEditorBin
{
  GstEditorElement element;

  /* lists of GUI elements and links */
  GList *elements, *links;

  /* where to make the next new element */
  gdouble element_x, element_y;

  /* datalist of GstElement names -> GstEditorItemAttr structs */
  GData **attributes;
};

struct _GstEditorBinClass
{
  GstEditorElementClass parent_class;
};



GType gst_editor_bin_get_type (void);
gdouble gst_editor_bin_sort (GstEditorBin * bin, gdouble step);
void gst_editor_bin_paste (GstEditorBin * bin);
void gst_editor_bin_debug_output (GstEditorBin * bin);

#endif /* __GST_EDITOR_BIN_H__ */

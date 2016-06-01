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


#ifndef __GST_EDITOR_PROPERTY_H__
#define __GST_EDITOR_PROPERTY_H__

#include <gst/gst.h>
#include <glade/glade.h>
#include <gst/editor/editor.h>

#define GST_TYPE_EDITOR_PROPERTY (gst_editor_property_get_type())
#define GST_EDITOR_PROPERTY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_EDITOR_PROPERTY, GstEditorProperty))
#define GST_EDITOR_PROPERTY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_EDITOR_PROPERTY, GstEditorPropertyClass))
#define GST_IS_EDITOR_PROPERTY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_EDITOR_PROPERTY))
#define GST_IS_EDITOR_PROPERTY_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_EDITOR_PROPERTY))
#define GST_EDITOR_PROPERTY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_EDITOR_PROPERTY, GstEditorPropertyClass))

#define GST_EDITOR_PROPERTY_SET_OBJECT(item,object) \
  (g_object_set_data (G_OBJECT(item), "gsteditorproperty", (object)))
#define GST_EDITOR_PROPERTY_GET_OBJECT(item) \
  (g_object_get_data (G_OBJECT(item),"gsteditorproperty"))

typedef struct _GstEditorProperty GstEditorProperty;
typedef struct _GstEditorPropertyClass GstEditorPropertyClass;

struct _GstEditorProperty
{
  GObject object;

  GstElement *shown_element;

  GladeXML *xml;
  GtkWidget *window;
  GtkWidget *element_ui;
  GtkWidget *caps_browser;
};

struct _GstEditorPropertyClass
{
  GObjectClass parent_class;

  void (*element_selected) (GstEditorProperty * property,
      GstEditorElement * element);
  void (*in_selection_mode) (GstEditorProperty * property,
      GstEditorElement * element);
};

GType gst_editor_property_get_type ();
GstEditorProperty *gst_editor_property_get ();

void gst_editor_property_show (GstEditorProperty * property,
    GstEditorElement * element);


#endif /* __GST_EDITOR_PROPERTY_H__ */

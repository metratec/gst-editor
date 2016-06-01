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


#include <gtk/gtk.h>
#include <gst/gst.h>

#include <gst/editor/editor.h>
#include <gst/common/gste-debug.h>
#include "gsteditorproperty.h"
#include "gsteditorpalette.h"
#include "gst-helper.h"


/* signals and args */
enum
{
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ATTRIBUTES,
  PROP_BIN,
  PROP_SELECTION,
  PROP_PROPERTIES_VISIBLE,
  PROP_PALETTE_VISIBLE,
  PROP_STATUS
};

static void gst_editor_canvas_class_init (GstEditorCanvasClass * klass);
static void gst_editor_canvas_init (GstEditorCanvas * editorcanvas);

static void gst_editor_canvas_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_editor_canvas_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_editor_canvas_dispose (GObject * object);

static void gst_editor_canvas_size_allocate (GtkWidget * widget,
    GtkAllocation * allocation);

static void on_realize (GstEditorCanvas * canvas);
static void on_property_destroyed (GstEditorCanvas * canvas,
    gpointer stale_pointer);
static void on_palette_destroyed (GstEditorCanvas * canvas,
    gpointer stale_pointer);

static GooCanvasClass *parent_class = NULL;

GType
gst_editor_canvas_get_type (void)
{
  static GType editor_canvas_type = 0;

  if (!editor_canvas_type) {
    static const GTypeInfo editor_canvas_info = {
      sizeof (GstEditorCanvasClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gst_editor_canvas_class_init,
      NULL,
      NULL,
      sizeof (GstEditorCanvas),
      0,
      (GInstanceInitFunc) gst_editor_canvas_init,
    };
    editor_canvas_type =
        g_type_register_static (GOO_TYPE_CANVAS, "GstEditorCanvas",
        &editor_canvas_info, 0);
  }
  return editor_canvas_type;
}

static void
gst_editor_canvas_class_init (GstEditorCanvasClass * klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = G_OBJECT_CLASS (klass);
  widget_class = (GtkWidgetClass *) klass;
  parent_class = g_type_class_ref (goo_canvas_get_type ());

  object_class->set_property = gst_editor_canvas_set_property;
  object_class->get_property = gst_editor_canvas_get_property;
  object_class->dispose = gst_editor_canvas_dispose;

  g_object_class_install_property (object_class, PROP_ATTRIBUTES,
      g_param_spec_pointer ("attributes", "attributes", "attributes",
          G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_BIN,
      g_param_spec_object ("bin", "bin", "bin",
          gst_bin_get_type (), G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SELECTION,
      g_param_spec_object ("selection", "selection", "selection",
          gst_editor_element_get_type (), G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PROPERTIES_VISIBLE,
      g_param_spec_boolean ("properties-visible", "properties-visible",
          "properties-visible", FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PALETTE_VISIBLE,
      g_param_spec_boolean ("palette-visible", "palette-visible",
          "palette-visible", FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_STATUS,
      g_param_spec_string ("status", "status", "status", "",
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  widget_class->size_allocate = gst_editor_canvas_size_allocate;
}

static void
gst_editor_canvas_init (GstEditorCanvas * editorcanvas)
{
  g_signal_connect_after (editorcanvas, "realize", G_CALLBACK (on_realize),
      NULL);
  g_static_rw_lock_init (&editorcanvas->globallock);
}

static void
gst_editor_canvas_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
  gdouble x, y, width, height;

  GooCanvasBounds bounds;
  GstEditorCanvas *canvas = GST_EDITOR_CANVAS (widget);

  if (canvas->bin) {
    width = allocation->width;
    height = allocation->height;
    if(canvas->autosize){
      g_object_set (canvas->bin, "width", width - 8, "height", height - 8, NULL);
      //g_object_set (canvas->bin, "width", width + 50, "height", height - 8, NULL);
      //g_print("gst_editor_canvas_size_allocate Allocation Size %f and %f\n",width,height);       goo_canvas_item_get_bounds (GOO_CANVAS_ITEM (canvas->bin), &bounds);
          x = bounds.x1;
          y = bounds.y1;
    
//    gnome_canvas_set_scroll_region (GNOME_CANVAS (canvas), x - 4, y - 4,
//        x + width - 4, y + height - 4);
 //       goo_canvas_set_bounds (GOO_CANVAS (canvas), x - 4, y - 4,
 //       x + width - 4, y + height - 4);
        goo_canvas_set_bounds (GOO_CANVAS (canvas), x - 4, y - 4,
        x + width -5, y + height - 5);
      gst_editor_on_spinbutton((GtkSpinButton *) NULL, canvas->parent);

     }
  //else{
    //g_print("gst_editor_canvas_size_allocate none\n");	

  //  }
  canvas->widthbackup=width - 8;
  canvas->heightbackup=height - 8;
  }
  if (GTK_WIDGET_CLASS (parent_class)->size_allocate) {
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
  }
}

static void
gst_editor_canvas_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GooCanvasBounds bounds;
  gdouble width, height, x, y;
  gboolean b;
  const gchar *status;
  GstEditorCanvas *canvas = GST_EDITOR_CANVAS (object);

  GooCanvasItem * bin;

  switch (prop_id) {
    case PROP_ATTRIBUTES:
      canvas->attributes = (GData **) g_value_get_pointer (value);
      EDITOR_DEBUG ("canvas_set_prop: attributesp: %p", canvas->attributes);
      break;
    case PROP_BIN:
      width = GTK_WIDGET (object)->allocation.width;
      height = GTK_WIDGET (object)->allocation.height;

      if (!canvas->bin) {
        bin =
            goo_canvas_item_new (GOO_CANVAS_ITEM (goo_canvas_get_root_item
                (GOO_CANVAS (canvas))), gst_editor_bin_get_type (),
"globallock", &canvas->globallock,            "attributes", canvas->attributes, "width", width, "height",
            height, "object", g_value_get_object (value), "resizeable", FALSE,
            "moveable", FALSE, NULL);
        EDITOR_DEBUG ("created a new bin canvas");
        gst_editor_bin_realize (bin);
        canvas->bin = GST_EDITOR_BIN (bin);
      } else {
        g_object_set (G_OBJECT (canvas->bin),
		"attributes", canvas->attributes, 		"object",g_value_get_object (value), 
		NULL);
        EDITOR_DEBUG ("replaced object on existing bin canvas and updated attributes");
      }
      goo_canvas_item_get_bounds (GOO_CANVAS_ITEM (canvas->bin), &bounds);
      x = bounds.x1;
      y = bounds.y1;
     
gdouble getx;
gdouble gety;
gdouble getscale;
gdouble getrotation;
goo_canvas_item_get_simple_transform(GOO_CANVAS_ITEM (canvas->bin),
&getx,
&gety,
&getscale,
&getrotation);

g_print("Gotten Transformation x: %f y:%f Scale %f Rotation %f and X: %f Y:%f\n",getx,gety,getscale,getrotation,x,y);
//goo_canvas_item_translate (GOO_CANVAS_ITEM (canvas->bin), -x, -y);
/*goo_canvas_item_set_simple_transform (GOO_CANVAS_ITEM (canvas->bin),
-x,
-y,
1.,
0.);*/
goo_canvas_item_set_simple_transform (GOO_CANVAS_ITEM (canvas->bin),
0.5,
0.5,
1.,
0.);
      g_object_set (canvas, "selection", canvas->bin, NULL);
      break;

    case PROP_SELECTION:
      if (canvas->selection)
        g_object_set (canvas->selection, "active", FALSE, NULL);

      canvas->selection = (GstEditorElement *) g_value_get_object (value);

      if (canvas->selection) {
        g_object_set (canvas->selection, "active", TRUE, NULL);
        if (canvas->property_window)
          g_object_set (canvas->property_window, "element",
              GST_EDITOR_ITEM (canvas->selection)->object, NULL);
      } else if (canvas->property_window) {
        g_object_set (canvas->property_window, "element", NULL, NULL);
      }

      break;

    case PROP_PROPERTIES_VISIBLE:
      g_return_if_fail (GTK_WIDGET_REALIZED (canvas) == TRUE);

      b = g_value_get_boolean (value);

      if (b) {
        if (!canvas->property_window) {
          canvas->property_window =
              g_object_new (gst_editor_property_get_type (), "parent",
              gtk_widget_get_toplevel (GTK_WIDGET (canvas)), NULL);
          g_object_weak_ref (G_OBJECT (canvas->property_window),
              (GWeakNotify) on_property_destroyed, canvas);
        }
        if (canvas->selection)
          g_object_set (canvas->property_window, "element",
              GST_EDITOR_ITEM (canvas->selection)->object, NULL);
      } else if (!b && canvas->property_window) {
        g_object_unref (G_OBJECT (canvas->property_window));
        /* the weak ref takes care of setting canvas->property_window = NULL */
      }

      break;

    case PROP_PALETTE_VISIBLE:
      g_return_if_fail (GTK_WIDGET_REALIZED (canvas) == TRUE);

      b = g_value_get_boolean (value);
      g_message ("palette visible: %s", b ? "TRUE" : "FALSE");

      if (b) {
        if (!canvas->palette) {
          g_message ("making new palette");
          canvas->palette =
              g_object_new (gst_editor_palette_get_type (), "canvas", canvas,
              NULL);
          g_object_weak_ref (G_OBJECT (canvas->palette),
              (GWeakNotify) on_palette_destroyed, canvas);
        }
      } else if (!b && canvas->palette) {
        g_message ("destroying palette");
        g_object_unref (G_OBJECT (canvas->palette));
        /* the weak ref takes care of setting canvas->palette = NULL */
      }

      break;

    case PROP_STATUS:
      status = g_value_get_string (value);

      g_return_if_fail (status != NULL);

      if (canvas->status)
        g_free (canvas->status);

      canvas->status = g_strdup (status);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_editor_canvas_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstEditorCanvas *canvas = GST_EDITOR_CANVAS (object);

  switch (prop_id) {
    case PROP_BIN:
      g_value_set_object (value,
          G_OBJECT (GST_EDITOR_ITEM (canvas->bin)->object));
      break;

    case PROP_SELECTION:
      g_value_set_object (value, (GObject *) canvas->selection);
      break;

    case PROP_PROPERTIES_VISIBLE:
      g_value_set_boolean (value, canvas->property_window ? TRUE : FALSE);
      break;

    case PROP_PALETTE_VISIBLE:
      g_value_set_boolean (value, canvas->palette ? TRUE : FALSE);
      break;

    case PROP_STATUS:
      g_value_set_string (value, g_strdup (canvas->status));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_editor_canvas_dispose (GObject * object)
{
  GstEditorCanvas *canvas = GST_EDITOR_CANVAS (object);

  if (canvas->property_window)
    g_object_unref (G_OBJECT (canvas->property_window));
  if (canvas->palette)
    g_object_unref (G_OBJECT (canvas->palette));
}

static void
on_realize (GstEditorCanvas * canvas)
{
  /* do this now so window manager hints can be set properly */
  //g_object_set (canvas,
  //    "properties-visible", TRUE, "palette-visible", TRUE, NULL);
  g_object_set (canvas,
      "properties-visible", TRUE, NULL);
}

static void
on_property_destroyed (GstEditorCanvas * canvas, gpointer stale_pointer)
{
  g_return_if_fail (GST_IS_EDITOR_CANVAS (canvas));

  canvas->property_window = NULL;

  g_object_notify (G_OBJECT (canvas), "properties-visible");
}

static void
on_palette_destroyed (GstEditorCanvas * canvas, gpointer stale_pointer)
{
  g_return_if_fail (GST_IS_EDITOR_CANVAS (canvas));

  canvas->palette = NULL;

  g_object_notify (G_OBJECT (canvas), "palette-visible");
}

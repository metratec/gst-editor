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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gst/gst.h>

#include <gst/editor/editor.h>
#include <gst/common/gste-debug.h>
#include "gsteditorproperty.h"
#include "gsteditorpalette.h"
#include "gsteditorelement.h"
#include "gsteditoritem.h"
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

static void gst_editor_canvas_element_connect (GstEditorCanvas * canvas,
    GstElement * pipeline);

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
  g_signal_connect_after (editorcanvas, "realize",
      G_CALLBACK (on_realize), NULL);
  g_rw_lock_init (&editorcanvas->globallock);

  g_datalist_init (&editorcanvas->attributes);
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
    if (canvas->autosize) {
      g_object_set (
          canvas->bin, "width", width - 8, "height", height - 8, NULL);
      // g_object_set (canvas->bin, "width", width + 50, "height", height - 8,
      // NULL);
      // g_print("gst_editor_canvas_size_allocate Allocation Size %f and
      // %f\n",width,height);
      goo_canvas_item_get_bounds (GOO_CANVAS_ITEM (canvas->bin), &bounds);
      x = bounds.x1;
      y = bounds.y1;

      goo_canvas_set_bounds (
          GOO_CANVAS (canvas), x - 4, y - 4, x + width - 5, y + height - 5);
      gst_editor_on_spinbutton ((GtkSpinButton *)NULL, canvas->parent);
    }
    // else{
    // g_print("gst_editor_canvas_size_allocate none\n");

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
  gdouble x, y;
  gboolean b;
  const gchar *status;
  GstEditorCanvas *canvas = GST_EDITOR_CANVAS (object);
  GtkAllocation allocation;

  GooCanvasItem * bin;

  switch (prop_id) {
    case PROP_ATTRIBUTES:
      g_datalist_clear (&canvas->attributes);
      canvas->attributes = (GData *) g_value_get_pointer (value);
      EDITOR_DEBUG ("canvas_set_prop: attributesp: %p", canvas->attributes);
      break;
    case PROP_BIN:
      gtk_widget_get_allocation (GTK_WIDGET (object), &allocation);

      if (!canvas->bin) {
        bin = goo_canvas_item_new (
            GOO_CANVAS_ITEM (goo_canvas_get_root_item (GOO_CANVAS (canvas))),
            gst_editor_bin_get_type (),
            "globallock", &canvas->globallock,
            "attributes", &canvas->attributes,
            "width", (gdouble)allocation.width,
            "height", (gdouble)allocation.height,
            "object", g_value_get_object (value),
            "resizeable", FALSE, "moveable", FALSE, NULL);
        EDITOR_DEBUG ("created a new bin canvas");
        gst_editor_bin_realize (bin);
        canvas->bin = GST_EDITOR_BIN (bin);
      } else {
        GstElement *pipeline = gst_editor_canvas_get_pipeline (canvas);

        /*
         * Need to clean up the GstBus signal watch.
         * See gst_editor_canvas_element_connect().
         */
        gst_bus_remove_signal_watch (gst_pipeline_get_bus (GST_PIPELINE (pipeline)));

        g_object_set (G_OBJECT (canvas->bin), "attributes", &canvas->attributes,
            "object", g_value_get_object (value), NULL);
        EDITOR_DEBUG (
            "replaced object on existing bin canvas and updated attributes");
      }
      goo_canvas_item_get_bounds (GOO_CANVAS_ITEM (canvas->bin), &bounds);
      x = bounds.x1;
      y = bounds.y1;

      gdouble getx;
      gdouble gety;
      gdouble getscale;
      gdouble getrotation;

      goo_canvas_item_get_simple_transform (
          GOO_CANVAS_ITEM (canvas->bin), &getx, &gety, &getscale, &getrotation);

      g_print (
          "Gotten Transformation x: %f y:%f Scale %f Rotation %f and X: %f "
          "Y:%f\n",
          getx, gety, getscale, getrotation, x, y);
      // goo_canvas_item_translate (GOO_CANVAS_ITEM (canvas->bin), -x, -y);
      /*goo_canvas_item_set_simple_transform (GOO_CANVAS_ITEM (canvas->bin),
      -x,
      -y,
      1.,
      0.);*/
      goo_canvas_item_set_simple_transform (
          GOO_CANVAS_ITEM (canvas->bin), 0.5, 0.5, 1., 0.);
      g_object_set (canvas, "selection", canvas->bin, NULL);

      gst_editor_canvas_element_connect (canvas,
          gst_editor_canvas_get_pipeline (canvas));
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

      /*
       * Update the PRIMARY selection clipboard.
       * TODO: Instead of always serializing the selected element, it is
       * possible to do this on-demand only since this selection property
       * is always kept up to date.
       * See gtk_clipboard_set_with_data().
       */
      gst_editor_element_copy (canvas->selection, GDK_SELECTION_PRIMARY);
      break;

    case PROP_PROPERTIES_VISIBLE:
      g_return_if_fail (gtk_widget_get_realized (GTK_WIDGET (canvas)) == TRUE);

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
      g_return_if_fail (gtk_widget_get_realized (GTK_WIDGET (canvas)) == TRUE);

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

  g_rw_lock_clear (&canvas->globallock);
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

static void
gst_editor_canvas_pipeline_message (GstBus * bus, GstMessage * message, gpointer data)
{
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_STATE_CHANGED:
    {
      GstObject *obj = GST_OBJECT (GST_MESSAGE_SRC (message));

      if (GST_IS_ELEMENT (obj)) {
        GstEditorItem *item = gst_editor_item_get (obj);
        //g_print("adding idle function in main bus watcher");
        g_idle_add ((GSourceFunc) gst_editor_element_sync_state,
            item /*editor_element */ );
      }
      break;
    }
    case GST_MESSAGE_EOS:
    {
      //printf("EOS Message from %s\n",GST_OBJECT_NAME(GST_OBJECT (GST_MESSAGE_SRC (message))));
      GstObject *obj = GST_OBJECT (GST_MESSAGE_SRC (message));

      if (GST_IS_ELEMENT (obj)) {
        gst_element_set_state (GST_ELEMENT_CAST (obj), GST_STATE_NULL);
        GstEditorItem * item = gst_editor_item_get (obj);
        // g_print("adding idle function in main bus watcher");
        gst_editor_element_stop_child (GST_EDITOR_ELEMENT (item));
        g_idle_add ((GSourceFunc)gst_editor_element_sync_state,
            item /*editor_element */);
      }
    }
    default:
      /* unhandled message */
      break;
  }
}

/* connect useful GStreamer signals to pipeline */
static void
gst_editor_canvas_element_connect (GstEditorCanvas * canvas, GstElement * pipeline)
{
  GstBus *bus;

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  /*
   * NOTE: The signal watch is cleaned up in the "bin" property
   * handler.
   */
  gst_bus_add_signal_watch (bus);
  g_signal_connect (bus, "message",
      G_CALLBACK (gst_editor_canvas_pipeline_message), canvas);
  gst_object_unref (bus);
}

static gboolean
parse_key_file_metadata (GKeyFile * key_file, GData ** datalistp, GError ** parent_error)
{
  GError *error = NULL;
  gchar **group_names = g_key_file_get_groups (key_file, NULL);

  for (gchar **group_name = group_names; *group_name; group_name++) {
    const gchar *element_name;
    GstEditorItemAttr *attr;

    if (!g_str_has_prefix (*group_name, "Element:"))
      continue;
    element_name = *group_name + 8;

    attr = g_new0 (GstEditorItemAttr, 1);

    attr->x = g_key_file_get_double (key_file, *group_name, "X", &error);
    if (error) {
      g_free (attr);
      goto propagate_error;
    }
    attr->y = g_key_file_get_double (key_file, *group_name, "Y", &error);
    if (error) {
      g_free (attr);
      goto propagate_error;
    }
    attr->w = g_key_file_get_double (key_file, *group_name, "Width", &error);
    if (error) {
      g_free (attr);
      goto propagate_error;
    }
    attr->h = g_key_file_get_double (key_file, *group_name, "Height", &error);
    if (error) {
      g_free (attr);
      goto propagate_error;
    }

    g_debug ("loaded %s with x: %f, y: %f, w: %f, h: %f",
        element_name, attr->x, attr->y, attr->w, attr->h);

    /* save this in the datalist with the object's name as key */
    g_datalist_set_data_full (datalistp, element_name, attr, g_free);
  }

  g_strfreev (group_names);
  return TRUE;

propagate_error:
  g_propagate_error (parent_error, error);
  g_strfreev (group_names);
  return FALSE;
}

gboolean
gst_editor_canvas_load_with_metadata (GstEditorCanvas * canvas,
    GKeyFile * key_file, GError ** error)
{
  gchar *str;
  GstElement *pipeline;
  GstEditorItemAttr *attr;

  /*
   * Check the save file version.
   * For the time being, we will only allow save files created with the
   * exact same version of GstEditor.
   */
  str = g_key_file_get_string (key_file, PACKAGE_NAME, "Version", NULL);
  if (g_strcmp0 (str, PACKAGE_VERSION) != 0) {
    g_set_error (error, GST_EDITOR_CANVAS_ERROR, GST_EDITOR_CANVAS_ERROR_FAILED,
        "Unsupported save file version %s", str);
    g_free (str);
    return FALSE;
  }
  g_free (str);

  /*
   * Parse the pipeline.
   */
  str = g_key_file_get_string (key_file, PACKAGE_NAME, "Pipeline", NULL);
  if (!str) {
    g_set_error (error, GST_EDITOR_CANVAS_ERROR, GST_EDITOR_CANVAS_ERROR_FAILED,
        "Missing %s/Pipeline in save file", PACKAGE_NAME);
    return FALSE;
  }
  /*
   * We use strict parsing here, since the pipeline
   * has been guaranteed to be programmatically written and
   * GsteSerialize should never produce faulty pipelines.
   * Otherwise GsteSerialize should be fixed instead.
   */
  pipeline = gst_parse_launch_full (str, NULL, GST_PARSE_FLAG_FATAL_ERRORS, error);
  g_free (str);
  if (!pipeline)
    /* forward error */
    return FALSE;

  /*
   * This properly frees all the elements.
   */
  g_datalist_clear (&canvas->attributes);

  /*
   * Parse the meta data into a GDatalist.
   * FIXME: It would be nice to avoid this.
   * At least it should be possible to use the data list that
   * GObjects include anyway.
   */
  if (!parse_key_file_metadata (key_file, &canvas->attributes, error)) {
    g_prefix_error (error, "Error parsing save file's metadata: ");
    return FALSE;
  }

  //first unref all the old stuff
  if (gst_editor_canvas_get_pipeline (canvas))
    g_object_unref (gst_editor_canvas_get_pipeline (canvas));

  EDITOR_DEBUG ("loaded: attributes: %p", canvas->attributes);

  g_object_set (canvas, "bin", pipeline, NULL);

  attr = g_datalist_get_data (&canvas->attributes, GST_ELEMENT_NAME (pipeline));
  /* now decide */
  if (attr) {
    gdouble x, y, width, height;
    GooCanvasBounds bounds;

    canvas->widthbackup = attr->w;
    canvas->heightbackup = attr->h;
    g_object_set (canvas->bin,
        "width", canvas->widthbackup,
        "height", canvas->heightbackup, NULL);
    if (canvas->bin)
      g_object_get (canvas->bin, "width", &width, "height", &height, NULL);

    goo_canvas_item_get_bounds (GOO_CANVAS_ITEM (canvas->bin), &bounds);
    x = bounds.x1;
    y = bounds.y1;
    goo_canvas_set_bounds (GOO_CANVAS (canvas),
        x - 4, y - 4, x + width + 3, y + height + 3);
  } else {
    g_warning ("Element attributes for %s not found!", GST_ELEMENT_NAME (pipeline));
  }

  return TRUE;
}

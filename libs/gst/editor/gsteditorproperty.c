/* gst-editor
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 *               <2002> Andy Wingo <wingo@pobox.com>
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

#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gst/gst.h>

#include "../common/gste-common.h"
#include "../element-ui/gst-element-ui.h"
#include "../element-browser/caps-tree.h"
#include "gsteditorproperty.h"

typedef struct
{
  GstEditorProperty *property;
  GModule *symbols;
}
connect_struct;

enum
{
  PROP_0,
  PROP_ELEMENT
};

enum
{
  LAST_SIGNAL
};

/* class functions */
static void gst_editor_property_class_init (GstEditorPropertyClass * klass);
static void gst_editor_property_init (GstEditorProperty * property);

static void gst_editor_property_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_editor_property_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_editor_property_connect_func (GtkBuilder * builder,
    GObject * object, const gchar * signal_name, const gchar * handler_name,
    GObject * connect_object, GConnectFlags flags, gpointer user_data);

static gpointer parent_class = NULL;

GType
gst_editor_property_get_type (void)
{
  static GType property_type = 0;

  if (!property_type) {
    static const GTypeInfo property_info = {
      sizeof (GstEditorPropertyClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gst_editor_property_class_init,
      NULL,
      NULL,
      sizeof (GstEditorProperty),
      0,
      (GInstanceInitFunc) gst_editor_property_init,
    };

    property_type =
        g_type_register_static (GTK_TYPE_BIN, "GstEditorProperty",
        &property_info, 0);
  }
  return property_type;
}

static void
gst_editor_property_class_init (GstEditorPropertyClass * klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_ref (GTK_TYPE_BIN);

  object_class->set_property = gst_editor_property_set_property;
  object_class->get_property = gst_editor_property_get_property;

  g_object_class_install_property (object_class, PROP_ELEMENT,
      g_param_spec_object ("element", "Element", "Element",
          gst_element_get_type (), G_PARAM_READWRITE));
}

static void
gst_editor_property_init (GstEditorProperty * property)
{
  connect_struct data;
  GModule *symbols;
  gchar *path;
  GtkWidget *notebook;
  GError *error = NULL;

  static const gchar *object_ids[] = {"property_notebook", NULL};

  symbols = g_module_open (NULL, 0);

  data.property = property;
  data.symbols = symbols;

  path = gste_get_ui_file ("editor.ui");
  if (!path)
    g_error ("GStreamer Editor user interface file 'editor.ui' not found.");

  property->builder = gtk_builder_new ();

  if (!gtk_builder_add_objects_from_file (property->builder,
          path, (gchar **) object_ids, &error)) {
    g_error (
        "GStreamer Editor could not load property_notebook from builder file: %s",
        error->message);
    g_error_free (error);
  }
  g_free (path);

  gtk_builder_connect_signals_full (property->builder,
      gst_editor_property_connect_func, &data);

  notebook =
      GTK_WIDGET (gtk_builder_get_object (property->builder, "property_notebook"));
  gtk_container_add (GTK_CONTAINER (property), notebook);

  property->element_ui =
      g_object_new (gst_element_ui_get_type (), "view-mode",
      GST_ELEMENT_UI_VIEW_MODE_FULL, NULL);
  gtk_widget_show (property->element_ui);
  gtk_container_add (
      GTK_CONTAINER (
          gtk_builder_get_object (property->builder, "scrolledwindow-element-ui")),
      property->element_ui);

  property->caps_browser =
      g_object_new (gst_element_browser_caps_tree_get_type (), NULL);
  gtk_container_add (
      GTK_CONTAINER (gtk_builder_get_object (
          property->builder, "scrolledwindow-caps-browser")),
      property->caps_browser);

  property->shown_element = NULL;
}

/* we need more control here so... */
static void
gst_editor_property_connect_func (GtkBuilder * builder,
    GObject * object,
    const gchar * signal_name,
    const gchar * handler_name,
    GObject * connect_object,
    GConnectFlags flags, gpointer user_data)
{
  gpointer func;
  connect_struct *data = (connect_struct *) user_data;

  if (!g_module_symbol (data->symbols, handler_name, &func))
    g_warning ("gsteditorproperty: could not find signal handler '%s'.",
        handler_name);
  else {
    if (flags & G_CONNECT_AFTER)
      g_signal_connect_after (object, signal_name, G_CALLBACK (func),
          (gpointer) data->property);
    else
      g_signal_connect (object, signal_name, G_CALLBACK (func),
          (gpointer) data->property);
  }
}

static void
gst_editor_property_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstEditorProperty *property;
  GstElement *old_element;

  /* get the major types of this object */
  property = GST_EDITOR_PROPERTY (object);
  old_element = property->shown_element;

  switch (prop_id) {
    case PROP_ELEMENT:
      property->shown_element = g_value_get_object (value);
      if (property->shown_element == old_element)
        break;

      g_object_set (property->element_ui, "element",
          property->shown_element, NULL);
      g_object_set (property->caps_browser, "element",
          property->shown_element, NULL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_editor_property_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstEditorProperty *property;

  /* get the major types of this object */
  property = GST_EDITOR_PROPERTY (object);

  switch (prop_id) {
    case PROP_ELEMENT:
      g_value_set_object (value, (GObject *) property->shown_element);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

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


#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gst/gst.h>
#include "gsteditorpalette.h"
#include "gsteditorpopup.h"
#include <gst/common/gste-common.h>
#include <gst/debug-ui/debug-ui.h>
#include <gst/element-browser/element-tree.h>
    typedef struct
{
  GstEditorPalette *palette;
  GModule *symbols;
}
connect_struct;

enum
{
  PROP_0,
  PROP_CANVAS
};

enum
{
  LAST_SIGNAL
};


/* class functions */
static void gst_editor_palette_class_init (GstEditorPaletteClass * klass);
static void gst_editor_palette_init (GstEditorPalette * palette);

static void gst_editor_palette_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_editor_palette_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_editor_palette_dispose (GObject * object);
static void gst_editor_palette_connect_func (const gchar * handler_name,
    GObject * object,
    const gchar * signal_name,
    const gchar * signal_data,
    GObject * connect_object, gboolean after, gpointer user_data);

static gint on_delete_event (GtkWidget * widget, GdkEvent * event,
    gpointer data);
static void on_element_tree_select (GstElementBrowserElementTree * element_tree,
    gpointer user_data);

static GtkObjectClass *parent_class;

/* static guint gst_editor_palette_signals[LAST_SIGNAL] = { 0 }; */


GType
gst_editor_palette_get_type (void)
{
  static GType palette_type = 0;

  if (!palette_type) {
    static const GTypeInfo palette_info = {
      sizeof (GstEditorPaletteClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gst_editor_palette_class_init,
      NULL,
      NULL,
      sizeof (GstEditorPalette),
      0,
      (GInstanceInitFunc) gst_editor_palette_init,
    };

    palette_type =
        g_type_register_static (G_TYPE_OBJECT, "GstEditorPalette",
        &palette_info, 0);
  }
  return palette_type;
}

static void
gst_editor_palette_class_init (GstEditorPaletteClass * klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_ref (G_TYPE_OBJECT);

  object_class->set_property = gst_editor_palette_set_property;
  object_class->get_property = gst_editor_palette_get_property;
  object_class->dispose = gst_editor_palette_dispose;

  g_object_class_install_property (object_class, PROP_CANVAS,
      g_param_spec_object ("canvas", "Canvas",
          "The EditorCanvas this palette is associated with",
          gst_editor_canvas_get_type (), G_PARAM_READWRITE));
}

static void
gst_editor_palette_init (GstEditorPalette * palette)
{
  connect_struct data;
  GModule *symbols;
  gchar *path;

  symbols = g_module_open (NULL, 0);

  data.palette = palette;
  data.symbols = symbols;

  path = gste_get_ui_file ("editor.glade2");
  if (!path)
    g_error ("GStreamer Editor user interface file 'editor.glade2' not found.");
  palette->xml = glade_xml_new (path, "utility_palette", NULL);

  if (!palette->xml) {
    g_error ("GStreamer Editor could not load utility_palette from %s", path);
  }
  g_free (path);
  g_assert (palette->xml != NULL);

  glade_xml_signal_autoconnect_full (palette->xml,
      gst_editor_palette_connect_func, &data);

  palette->window = glade_xml_get_widget (palette->xml, "utility_palette");
  palette->element_tree =
      g_object_new (gst_element_browser_element_tree_get_type (), NULL);

  gtk_box_pack_start (GTK_BOX (glade_xml_get_widget (palette->xml,
              "element-browser-vbox")), palette->element_tree, TRUE, TRUE, 0);
  g_signal_connect (palette->element_tree, "element-activated",
      G_CALLBACK (on_element_tree_select), palette);
  
#ifndef _MSC_VER
      palette->debug_ui = gst_debug_ui_new ();
  gtk_box_pack_start (GTK_BOX (glade_xml_get_widget (palette->xml,
              "debug-vbox")), palette->debug_ui, TRUE, TRUE, 0);
#endif
  g_signal_connect (palette->window, "delete-event",
      G_CALLBACK (on_delete_event), palette);

  gtk_widget_show_all (palette->window);
}

/* we need more control here so... */
static void
gst_editor_palette_connect_func (const gchar * handler_name,
    GObject * object,
    const gchar * signal_name,
    const gchar * signal_data,
    GObject * connect_object, gboolean after, gpointer user_data)
{
  gpointer func;
  connect_struct *data = (connect_struct *) user_data;

  if (!g_module_symbol (data->symbols, handler_name, &func))
    g_warning ("gsteditorpalette: could not find signal handler '%s'.",
        handler_name);
  else {
    if (after)
      g_signal_connect_after (object, signal_name, (GCallback) func,
          (gpointer) data->palette);
    else
      g_signal_connect (object, signal_name, (GCallback) func,
          (gpointer) data->palette);
  }
}

static void
gst_editor_palette_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GdkAtom atoms[2] = { GDK_NONE, GDK_NONE };
  GdkWindow *window;
  GstEditorPalette *palette = GST_EDITOR_PALETTE (object);

  switch (prop_id) {
    case PROP_CANVAS:
      palette->canvas = (GstEditorCanvas *) g_value_get_object (value);
      if (GTK_IS_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (palette->
                      canvas)))) {
        gtk_window_set_transient_for (GTK_WINDOW (palette->window),
            GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (palette->
                        canvas))));

        /* we are assumed to be realized at this point.. */
        window = palette->window->window;

        atoms[0] = gdk_atom_intern ("_NET_WM_WINDOW_TYPE_UTILITY", FALSE);
        gdk_property_change (window, gdk_atom_intern ("_NET_WM_WINDOW_TYPE",
                FALSE), gdk_atom_intern ("ATOM", FALSE), 32,
            GDK_PROP_MODE_REPLACE, (guchar *) atoms, 1);
      }

      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_editor_palette_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstEditorPalette *palette = GST_EDITOR_PALETTE (object);

  switch (prop_id) {
    case PROP_CANVAS:
      g_value_set_object (value, (GObject *) palette->canvas);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_editor_palette_dispose (GObject * object)
{
  GstEditorPalette *palette = GST_EDITOR_PALETTE (object);

  gtk_widget_destroy (palette->window);

  if (G_OBJECT_CLASS (parent_class)->dispose)
    G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gint
on_delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  GstEditorPalette *palette = GST_EDITOR_PALETTE (data);

  g_object_unref (G_OBJECT (palette));

  return FALSE;
}

static void
on_element_tree_select (GstElementBrowserElementTree * element_tree,
    gpointer user_data)
{
  GstElement *element, *selected_bin;
  GstElementFactory *selected_factory;
  GstEditorPalette *palette = GST_EDITOR_PALETTE (user_data);

  g_return_if_fail (palette->canvas != NULL);
  g_object_get (element_tree, "selected", &selected_factory, NULL);
  g_object_get (palette->canvas, "selection", &selected_bin, NULL);

  /* GstEditorCanvas::selection is actually a GstEditorElement, not a
     GstElement */
  if (selected_bin)
    selected_bin = GST_ELEMENT (GST_EDITOR_ITEM (selected_bin)->object);

  if (!selected_bin)
    selected_bin = GST_ELEMENT (GST_EDITOR_ITEM (palette->canvas->bin)->object);
  else if (!GST_IS_BIN (selected_bin))
    selected_bin = GST_ELEMENT (GST_OBJECT_PARENT (selected_bin));

  /* Check if we're allowed to add to the bin, ie if it's paused.
   * if not, throw up a warning */
  if (gst_element_get_state (selected_bin, NULL, NULL,
          GST_CLOCK_TIME_NONE) == GST_STATE_PLAYING) {
    gchar *message =
        g_strdup_printf ("bin %s is in PLAYING state, you cannot add "
        "elements to it in this state !",
        gst_element_get_name (selected_bin));

    gst_editor_popup_warning (message);
    g_free (message);
    return;
  }

  element = gst_element_factory_create (selected_factory, NULL);

  g_return_if_fail (element != NULL);
  gst_bin_add (GST_BIN (selected_bin), element);
}

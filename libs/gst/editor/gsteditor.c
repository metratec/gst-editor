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

#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <goocanvas.h>

#include "gsteditor.h"
#include "gsteditoritem.h"
#include "gsteditorcanvas.h"
#include "gsteditorelement.h"
#include "gsteditorstatusbar.h"
#include "namedicons.h"

#include <gst/common/gste-common.h>
#include <gst/common/gste-debug.h>
#include <gst/common/gste-serialize.h>
#include "../element-browser/browser.h"
#include <gst/element-browser/element-tree.h>

#define GST_CAT_DEFAULT gste_debug_cat

static void gst_editor_class_init (GstEditorClass * klass);
static void gst_editor_init (GstEditor * project_editor);
static void gst_editor_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_editor_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_editor_dispose (GObject * object);
static void gst_editor_finalize (GObject * object);
static void gst_editor_connect_func (GtkBuilder * builder, GObject * object,
    const gchar * signal_name, const gchar * handler_name,
    GObject * connect_object, GConnectFlags flags, gpointer user_data);

static gint on_delete_event (GtkWidget * widget,
    GdkEvent * event, GstEditor * editor);
static void on_canvas_notify (GObject * object,
    GParamSpec * param, GstEditor * ui);
static void on_element_tree_select (GstElementBrowserElementTree * element_tree,
    gpointer user_data);

enum
{
  PROP_0,
  PROP_FILENAME
};

enum
{
  LAST_SIGNAL
};

typedef struct
{
  GstEditor *editor;
  GModule *symbols;
}
connect_struct;


static GObjectClass *parent_class;

static gint _num_editor_windows = 0;

/* error dialog response - FIXME: we can use positive values since the
   GTK_RESPONSE ones are negative ? */
#define GST_EDITOR_RESPONSE_DEBUG 1

GType
gst_editor_get_type (void)
{
  static GType project_editor_type = 0;

  if (!project_editor_type) {
    static const GTypeInfo project_editor_info = {
      sizeof (GstEditorClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gst_editor_class_init,
      NULL,
      NULL,
      sizeof (GstEditor),
      0,
      (GInstanceInitFunc) gst_editor_init,
    };

    project_editor_type =
        g_type_register_static (G_TYPE_OBJECT, "GstEditor",
        &project_editor_info, 0);
  }
  return project_editor_type;
}

static void
gst_editor_class_init (GstEditorClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_ref (G_TYPE_OBJECT);

  gobject_class->get_property = gst_editor_get_property;
  gobject_class->set_property = gst_editor_set_property;
  gobject_class->dispose = gst_editor_dispose;
  gobject_class->finalize = gst_editor_finalize;

  g_object_class_install_property (gobject_class, PROP_FILENAME,
      g_param_spec_string ("filename", "Filename", "File name",
          NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  gst_editor_named_icons_init ();

  /*
   * FIXME: This will affect the entire application,
   * even if it just embeds the GstEditor.
   */
  gtk_window_set_default_icon_name (GST_EDITOR_NAMED_ICON_LOGO);
}

static void
gst_editor_init (GstEditor * editor)
{
  connect_struct data;
  GModule *symbols;
  gchar *path;
  GError *error = NULL;

  static const gchar *object_ids[] = {
      "main_project_window", "adjustment1", "adjustment2",
      "save_dialog", "save_dialog_settings", NULL
  };

  symbols = g_module_open (NULL, 0);

  data.editor = editor;
  data.symbols = symbols;

  path = gste_get_ui_file ("editor.ui");
  if (!path)
    g_error ("GStreamer Editor user interface file 'editor.ui' not found.");

  editor->builder = gtk_builder_new ();

  if (!gtk_builder_add_objects_from_file (editor->builder,
          path, (gchar **) object_ids, &error)) {
    g_error (
        "GStreamer Editor could not load main_project_window from builder "
        "file: %s",
        error->message);
    g_error_free (error);
  }
  g_free (path);

  gtk_builder_connect_signals_full (editor->builder,
      gst_editor_connect_func, &data);

  editor->save_dialog =
      GTK_WIDGET (gtk_builder_get_object (editor->builder, "save_dialog"));

  /*
   * NOTE: Filter support is more or less broken in Glade, so
   * we add them with code here.
   * Filter ownership is transferred to the open/save dialogs,
   * so we do not have to clean them up.
   */
  editor->filter_gep = gtk_file_filter_new ();
  gtk_file_filter_set_name (editor->filter_gep,
      _("Gst-Editor Pipeline (*.gep)"));
  gtk_file_filter_add_pattern (editor->filter_gep, "*.gep");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (editor->save_dialog),
      editor->filter_gep);

  editor->filter_gsp = gtk_file_filter_new ();
  gtk_file_filter_set_name (editor->filter_gsp,
      _("Plain Gst-Launch Pipeline (*.gsp;*.txt)"));
  gtk_file_filter_add_pattern (editor->filter_gsp, "*.gsp");
  gtk_file_filter_add_pattern (editor->filter_gsp, "*.txt");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (editor->save_dialog),
      editor->filter_gsp);

  editor->window =
      GTK_WIDGET (gtk_builder_get_object (editor->builder, "main_project_window"));

  editor->sw = GTK_SPIN_BUTTON (gtk_builder_get_object (editor->builder, "spinbutton1"));
  editor->sh = GTK_SPIN_BUTTON (gtk_builder_get_object (editor->builder, "spinbutton2"));

  gtk_widget_show (editor->window);
//Code for element tree
  editor->element_tree =
      g_object_new (gst_element_browser_element_tree_get_type (), NULL);
  gtk_box_pack_start (GTK_BOX (gtk_builder_get_object (editor->builder,
              "main_element_tree_box")), editor->element_tree, TRUE, TRUE, 0);
  g_signal_connect (editor->element_tree, "element-activated",
      G_CALLBACK (on_element_tree_select), editor);
  gtk_widget_show (editor->element_tree);
//finished



  editor->canvas =
      (GstEditorCanvas *) g_object_new (GST_TYPE_EDITOR_CANVAS, NULL);
  editor->canvas->autosize =TRUE;
  editor->canvas->parent=editor;
  gtk_widget_show (GTK_WIDGET (editor->canvas));

  gtk_container_add (GTK_CONTAINER (gtk_builder_get_object (editor->builder,
              "canvasSW")), GTK_WIDGET (editor->canvas));

  _num_editor_windows++;

  gst_editor_statusbar_init (editor);

  g_signal_connect (editor->window, "delete-event",
      G_CALLBACK (on_delete_event), editor);
  g_signal_connect (editor->canvas, "notify", G_CALLBACK (on_canvas_notify),
      editor);
  g_mutex_init (&editor->outputmutex);
}

static void
gst_editor_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  gchar *title;
  GstEditor *editor = GST_EDITOR (object);
  const gchar *filename;
  static gint count = 1;

  switch (prop_id) {
    case PROP_FILENAME:
      if (editor->filename)
        g_free (editor->filename);
      filename = g_value_get_string (value);
      if (!filename) {
        editor->filename = g_strdup_printf ("untitled-%d.gep", count++);
        editor->need_name = TRUE;
      } else {
        editor->filename = g_strdup (filename);
        editor->need_name = FALSE;
      }

      title = g_strdup_printf ("%s%s - %s",
          editor->filename, editor->changed ? "*" : "",
          _("GStreamer Pipeline Editor"));
      gtk_window_set_title (GTK_WINDOW (editor->window), title);
      g_free (title);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_editor_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstEditor *editor = GST_EDITOR (object);

  switch (prop_id) {
    case PROP_FILENAME:
      g_value_set_string (value, g_strdup (editor->filename));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_editor_dispose (GObject * object)
{
  GstEditor *editor = GST_EDITOR (object);

  gtk_widget_destroy (editor->window);

  _num_editor_windows--;

  g_message ("editor dispose: %d windows remaining", _num_editor_windows);

  if (_num_editor_windows == 0)
    gtk_main_quit ();

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_editor_finalize (GObject * object)
{
  GstEditor *editor = GST_EDITOR (object);

  g_mutex_clear (&editor->outputmutex);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/*
 * helper functions
 */

/* show an error dialog with a gerror and debug string */
static void
gst_editor_dialog_gerror (GtkWindow * window, GstMessage* message)
{
  GtkWidget *error_dialog, *button, *image;
  gint response;
//getting GError

  GError *err;
  gchar *debug, *tmp;
  gchar *name =
    gst_object_get_path_string (GST_OBJECT (GST_MESSAGE_SRC (message)));

  gst_message_parse_error (message, &err, &debug);
  tmp = err->message;
  err->message = g_strdup_printf ("%s: %s", name, err->message);
  g_free (tmp);

  g_return_if_fail (err);

  error_dialog = gtk_message_dialog_new (window, GTK_DIALOG_MODAL,
      GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, "%s", err->message);
  /* add a stop pipeline button */
//  gtk_dialog_add_button (GTK_DIALOG (error_dialog), "STOP",
//      GTK_RESPONSE_CANCEL);
  gtk_dialog_add_button (GTK_DIALOG (error_dialog),
      _("_Cancel"), GTK_RESPONSE_CANCEL);
  /* add a debug button if there is debug info */
  if (debug) {
    button = gtk_dialog_add_button (GTK_DIALOG (error_dialog),
        _("_Debug"), GST_EDITOR_RESPONSE_DEBUG);
    image = gtk_image_new_from_icon_name ("dialog-information",
        GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image (GTK_BUTTON (button), image);
  }
  /* make sure OK is on the right and default action to be HIG-compliant */
  gtk_dialog_add_button (GTK_DIALOG (error_dialog),
      _("_OK"), GTK_RESPONSE_OK);

  gtk_dialog_set_default_response (GTK_DIALOG (error_dialog), GTK_RESPONSE_OK);
  response = gtk_dialog_run (GTK_DIALOG (error_dialog));

  if (response == GTK_RESPONSE_CANCEL) {
    GstObject *obj = GST_OBJECT (GST_MESSAGE_SRC (message));
    if (GST_IS_ELEMENT (obj)) {
      GstElement * top = GST_ELEMENT (obj);
      while (GST_ELEMENT_PARENT (top))
        top = GST_ELEMENT_PARENT (top);
      // g_print("GstEditor: stopping on demand\n");
      gst_element_set_state (top, GST_STATE_NULL);
      GstEditorItem *item = gst_editor_item_get (GST_OBJECT (top));
      gst_editor_element_stop_child (GST_EDITOR_ELEMENT (item));
      // g_idle_add ((GSourceFunc) gst_editor_element_sync_state,
      //    item /*editor_element */ );
    }
  } else if (response == GST_EDITOR_RESPONSE_DEBUG) {
    gtk_widget_destroy (error_dialog);
    error_dialog = gtk_message_dialog_new (GTK_WINDOW (window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "%s\n\nAdditional debug info: %s\n", err->message, debug);
    gtk_dialog_run (GTK_DIALOG (error_dialog));
  }
  g_error_free (err);
  g_free (debug);
  gtk_widget_destroy (error_dialog);
}

/* GStreamer callbacks */

static void
gst_editor_pipeline_message (GstBus * bus, GstMessage * message, gpointer data)
{
  GstEditor *editor = GST_EDITOR (data);

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
      gst_editor_dialog_gerror (GTK_WINDOW (editor->window), message);
      break;

    default:
      /* unhandled message */
      break;
  }
}

/* connect useful GStreamer signals to pipeline */
static void
gst_editor_element_connect (GstEditor * editor, GstElement * pipeline)
{
  GstBus *bus;

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  /*
   * NOTE: The signal watch is cleaned up in gst_editor_load().
   */
  gst_bus_add_signal_watch (bus);
  g_signal_connect (bus, "message",
      G_CALLBACK (gst_editor_pipeline_message), editor);
  gst_object_unref (bus);
}

/* we need more control here so... */
static void
gst_editor_connect_func (GtkBuilder * builder,
    GObject * object,
    const gchar * signal_name,
    const gchar * handler_name,
    GObject * connect_object,
    GConnectFlags flags, gpointer user_data)
{
  gpointer func;
  connect_struct *data = (connect_struct *) user_data;

  if (!g_module_symbol (data->symbols, handler_name, &func))
    g_warning ("GstEditor: could not find signal handler '%s'.", handler_name);
  else {
    if (flags & G_CONNECT_AFTER)
      g_signal_connect_after (object, signal_name, G_CALLBACK (func),
          (gpointer) data->editor);
    else
      g_signal_connect (object, signal_name, G_CALLBACK (func),
          (gpointer) data->editor);
  }
}

GtkWidget *
gst_editor_new (GstElement * element)
{
  GtkWidget *ret = g_object_new (gst_editor_get_type (), NULL);

  if (element) {
    g_object_set (GST_EDITOR (ret)->canvas, "bin", element, NULL);
    gst_editor_element_connect (GST_EDITOR (ret), element);
  }

  return ret;
}

static gboolean
gst_editor_save (GstEditor * editor)
{
  GError *error = NULL;
  GstEditorItem *pipeline_item = GST_EDITOR_ITEM (editor->canvas->bin);

  if (g_str_has_suffix (editor->filename, ".gep")) {
    /*
     * Save as Gst-Editor Pipeline with metadata.
     */
    GKeyFile *key_file = g_key_file_new ();

    gst_editor_item_save_with_metadata (pipeline_item, key_file,
        editor->save_flags);

    g_key_file_save_to_file (key_file, editor->filename, &error);
    g_key_file_unref (key_file);
    if (error) {
      g_warning ("%s could not be saved: %s", editor->filename, error->message);
      g_error_free (error);
      return FALSE;
    }
  } else {
    /*
     * Save as plain Gst-Launch pipeline.
     */
    gchar *pipeline = gst_editor_item_save (pipeline_item, editor->save_flags);

    g_file_set_contents (editor->filename, pipeline, -1, &error);
    g_free (pipeline);
    if (error) {
      g_warning ("%s could not be saved: %s", editor->filename, error->message);
      g_error_free (error);
      return FALSE;
    }
  }

  gst_editor_statusbar_message ("Pipeline saved to %s.", editor->filename);
  return TRUE;
}

void
gst_editor_on_save_as (GtkWidget * widget, GstEditor * editor)
{
  gint response;
  gchar *filename;
  GObject *setting;

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (editor->save_dialog),
      g_str_has_suffix (editor->filename, ".gep")
          ? editor->filter_gep : editor->filter_gsp);

  if (editor->need_name)
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (editor->save_dialog),
        editor->filename);
  else
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (editor->save_dialog),
        editor->filename);

  /*
   * NOTE: The dialog's response codes have been chosen manually in Glade
   * to match GTK_RESPONSE_ACCEPT/GTK_RESPONSE_CANCEL.
   * Let's just hope the stock response Ids do not change suddenly.
   */
  response = gtk_dialog_run (GTK_DIALOG (editor->save_dialog));
  gtk_widget_hide (editor->save_dialog);
  if (response != GTK_RESPONSE_ACCEPT)
    return;

  filename =
      gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (editor->save_dialog));
  g_object_set (editor, "filename", filename, NULL);
  g_free (filename);

  /*
   * Determine flags from the dialog's setting check boxes.
   * NOTE: save_dialog_capsfilter_as_element is enabled by default
   * since that preserves more information and the save files (at least
   * the *.gep files) are not meant for human consumption anyway.
   * Other setting flags might be enforced by the serialization.
   */
  editor->save_flags = 0;
  setting = gtk_builder_get_object (editor->builder, "save_dialog_verbose");
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setting)))
    editor->save_flags |= GSTE_SERIALIZE_VERBOSE;
  setting = gtk_builder_get_object (editor->builder, "save_dialog_all_bins");
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setting)))
    editor->save_flags |= GSTE_SERIALIZE_ALL_BINS;
  setting = gtk_builder_get_object (editor->builder, "save_dialog_pipelines_as_bins");
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setting)))
    editor->save_flags |= GSTE_SERIALIZE_PIPELINES_AS_BINS;
  setting = gtk_builder_get_object (editor->builder, "save_dialog_capsfilter_as_element");
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (setting)))
    editor->save_flags |= GSTE_SERIALIZE_CAPSFILTER_AS_ELEMENT;

  gst_editor_save (editor);
}

void
gst_editor_on_save (GtkWidget * widget, GstEditor * editor)
{
  if (editor->need_name) {
    gst_editor_on_save_as (NULL, editor);
    return;
  }

  /*
   * Inherits file name and flags from the last save dialog
   * invocation.
   */
  gst_editor_save (editor);
}

void
gst_editor_load (GstEditor * editor, const gchar * file_name)
{
  GError *error = NULL;
  GKeyFile *key_file = g_key_file_new ();
  GstElement *pipeline;

  if (!g_key_file_load_from_file (key_file, file_name, G_KEY_FILE_NONE, &error)) {
    g_warning ("Error parsing save file \"%s\": %s", file_name, error->message);
    g_error_free (error);
    goto cleanup;
  }

  pipeline = gst_editor_canvas_get_pipeline (editor->canvas);
  if (pipeline)
    gst_bus_remove_signal_watch (gst_pipeline_get_bus (GST_PIPELINE (pipeline)));

  if (!gst_editor_canvas_load_with_metadata (editor->canvas, key_file, &error)) {
    g_warning ("Error loading key file: %s", error->message);
    g_error_free (error);
    goto cleanup;
  }

  g_object_set (editor, "filename", file_name, NULL);

  gst_editor_statusbar_message ("Pipeline loaded from %s.", editor->filename);

  pipeline = gst_editor_canvas_get_pipeline (editor->canvas);
  gst_editor_element_connect (editor, pipeline);

cleanup:
  g_key_file_unref (key_file);
}

void
gst_editor_on_open (GtkWidget * widget, GstEditor * editor)
{
  GtkWidget *file_chooser;

  /*
   * NOTE: Could be moved into Glade (editor.ui) just like the Save dialog,
   * but there is little benefit for this one.
   */
  file_chooser = gtk_file_chooser_dialog_new ("Please select a file to load.",
      GTK_WINDOW (editor->window), GTK_FILE_CHOOSER_ACTION_OPEN,
      _("_Cancel"), GTK_RESPONSE_CANCEL,
      _("_Open"), GTK_RESPONSE_ACCEPT,
      NULL);

  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), editor->filter_gep);
  /*
   * TODO: Currently we can only load pipelines with meta data.
   * It might be useful to open plain pipeline definitions as well.
   * However, this requires support for automatically laying out the pipeline graph.
   */
  //gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), editor->filter_gsp);
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (file_chooser), editor->filter_gep);

  if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) {
    gchar *filename =
        gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
    gst_editor_load (editor, filename);
    g_free (filename);
  }

  gtk_widget_destroy (file_chooser);
}

void
gst_editor_on_new_empty_pipeline (GtkWidget * widget, GstEditor * editor)
{
  gst_editor_new (gst_element_factory_make ("pipeline", NULL));
}

#if 0
static void
have_pipeline_description (gchar * string, gpointer data)
{
  GstElement *pipeline;
  GError *error = NULL;
  GstEditor *editor = NULL;

  if (!string)
    return;

  pipeline = (GstElement *) gst_parse_launch (string, &error);

  if (!pipeline) {
    GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (data),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_CLOSE,
        "Pipeline failed to parse: %s", error->message);

    gtk_widget_show (dialog);
    g_signal_connect_swapped (dialog, "response",
        G_CALLBACK (gtk_widget_destroy), dialog);

    return;
  }

  editor = (GstEditor *) gst_editor_new (pipeline);

  gst_editor_statusbar_message ("Pipeline loaded from launch description.");
}
#endif

void
gst_editor_on_new_from_pipeline_description (GtkWidget * widget,
    GstEditor * editor)
{
  static GtkWidget *request = NULL;
  GtkWidget *label;

//  GtkWidget *entry;

  if (!request) {
    request = gtk_dialog_new_with_buttons ("Gst-Editor",
        GTK_WINDOW (editor->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        _("_OK"), GTK_RESPONSE_ACCEPT,
        _("_Cancel"), GTK_RESPONSE_REJECT,
        NULL);
    label = gtk_label_new ("Please enter in a pipeline description. "
        "See the gst-launch man page for help on the syntax.");
    gtk_container_add (
        GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (request))),
        label);
    /* set up text entry widget */
    /* entry = gtk_entry_new();
       gtk_entry_set_text(entry, "fakesrc ! fakesink");
       gtk_box_pack_end ( GTK_BOX(GTK_DIALOG(dialog)->vbox),
       entry, FALSE, FALSE, GNOME_PAD_SMALL );
       ...
     */

    g_signal_connect_swapped (request, "response",
        G_CALLBACK (gtk_widget_hide), request);
  }

  gtk_widget_show (request);
}

void
gst_editor_on_close (GtkWidget * widget, GstEditor * editor)
{
  g_object_unref (G_OBJECT (editor));
}

void
gst_editor_on_quit (GtkWidget * widget, GstEditor * editor)
{
  gtk_main_quit ();
}

void
gst_editor_on_cut (GtkWidget * widget, GstEditor * editor)
{
  GstEditorElement *element = NULL;

  g_object_get (editor->canvas, "selection", &element, NULL);

  if (!element) {
    gst_editor_statusbar_message
        ("Could not cut element: No element currently selected.");
    return;
  }

  gst_editor_element_cut (element);
}

void
gst_editor_on_copy (GtkWidget * widget, GstEditor * editor)
{
  GstEditorElement *element = NULL;

  g_object_get (editor->canvas, "selection", &element, NULL);

  if (!element) {
    gst_editor_statusbar_message
        ("Could not copy element: No element currently selected.");
    return;
  }

  gst_editor_element_copy (element);
}

void
gst_editor_on_paste (GtkWidget * widget, GstEditor * editor)
{
  gst_editor_bin_paste (editor->canvas->bin);
}

void
gst_editor_on_add (GtkWidget * widget, GstEditor * editor)
{
  GstElementFactory *factory;
  GstElement *element;

  if ((factory = gst_element_browser_pick_modal ())) {
    if (!(element = gst_element_factory_create (factory, NULL))) {
      g_warning ("unable to create element of type '%s'",
          GST_OBJECT_NAME (factory));
      return;
    }
    /* the object_added signal takes care of drawing the gui */
    gst_bin_add (GST_BIN (GST_EDITOR_ITEM (editor->canvas->bin)->object),
        element);
  }
}

void
gst_editor_on_remove (GtkWidget * widget, GstEditor * editor)
{
  GstEditorElement *element = NULL;

  g_object_get (editor->canvas, "selection", &element, NULL);

  if (!element) {
    gst_editor_statusbar_message
        ("Could not remove element: No element currently selected.");
    return;
  }

  gst_editor_element_remove (element);
}

void
gst_editor_show_element_inspector (GtkWidget * widget, GstEditor * editor)
{
  gboolean b;

  g_object_get (widget, "active", &b, NULL);
  g_object_set (editor->canvas, "properties-visible", b, NULL);
}

void
gst_editor_show_utility_palette (GtkWidget * widget, GstEditor * editor)
{
  gboolean b;

  g_object_get (widget, "active", &b, NULL);

  g_message ("show utility palette: %d", b);

  g_object_set (editor->canvas, "palette-visible", b, NULL);
}

void
gst_editor_on_help_contents (GtkWidget * widget, GstEditor * editor)
{
  GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (editor->window),
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_WARNING,
      GTK_BUTTONS_CLOSE,
      "Open help contents !!!");

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

void
gst_editor_on_about (GtkWidget * widget, GstEditor * editor)
{
  static const gchar *authors[] = {
      "Hannes Bistry", "Andy Wingo", "Erik Walthinsen",
      "Jan Schmidt", "Robin Haberkorn", NULL
  };

  gtk_show_about_dialog (GTK_WINDOW (editor->window),
      "name", "GStreamer Pipeline Editor",
      "version", VERSION,
      "copyright",
      "(c) 2001-2007 GStreamer Team, 2008-2010 Hannes Bistry, 2016 metraTec GmbH",
      "license",
      "This library is free software; you can redistribute it and/or "
      "modify it under the terms of the GNU Library General Public "
      "License as published by the Free Software Foundation; either "
      "version 2 of the License, or (at your option) any later version.\n"
      "\n"
      "This library is distributed in the hope that it will be useful, "
      "but WITHOUT ANY WARRANTY; without even the implied warranty of "
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU "
      "Library General Public License for more details.\n"
      "\n"
      "You should have received a copy of the GNU Library General Public "
      "License along with this library; if not, write to the "
      "Free Software Foundation, Inc., 59 Temple Place - Suite 330, "
      "Boston, MA 02111-1307, USA.",
      "wrap-license", TRUE,
      "comments",
      "A graphical pipeline editor for "
      "GStreamer capable of loading and saving GstParse/gst-launch pipelines.",
      "website", "https://github.com/metratec/gst-editor/",
      "authors", authors,
      "logo-icon-name", GST_EDITOR_NAMED_ICON_LOGO,
      NULL);
}

#if 0
static gboolean
sort (GstEditor * editor)
{
  gst_editor_statusbar_message ("Change in bin pressure: %f",
      gst_editor_bin_sort (editor->canvas->bin, 0.1));
  return TRUE;
}
#endif

//callback that generates an output message to stderr
static GstPadProbeReturn
cb_output (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstBuffer * buffer = GST_PAD_PROBE_INFO_BUFFER (info);
  GstEditor * editor = GST_EDITOR (user_data);
  guint64 time;
  //fprintf(stderr,"Buffer:)");
  struct timeval now;

  gettimeofday (&now, NULL);
  time = ((now.tv_sec*1000000000ULL)+(now.tv_usec*1000ULL));

  g_mutex_lock (&editor->outputmutex);
  if (GST_STATE (GST_OBJECT (pad)->parent) == GST_STATE_PLAYING) {
    fprintf (stderr, "%s %" G_GUINT64_FORMAT " %" G_GUINT64_FORMAT " "
                     "%" G_GSIZE_FORMAT " "
                     "%" G_GUINT64_FORMAT " %ld %d %lu %s\n",
        GST_OBJECT (pad)->parent->name, GST_BUFFER_PTS (buffer), time,
        gst_buffer_get_size (buffer), GST_BUFFER_OFFSET (buffer), clock (),
        getpid (), pthread_self (), GST_OBJECT (pad->peer)->parent->name);
    // Element Name, timestamp, timenow, jiffies now,Prozessnr, threadnr,Peer
    // Element Name
  }
  g_mutex_unlock (&editor->outputmutex);

  return GST_PAD_PROBE_OK;
}


void
gst_editor_on_sort_toggled (GtkToggleButton * toggle, GstEditor * editor)
{
  GList *elements, *srcpads;
    
    //gst_editor_statusbar_message ("Doing debug output...");
    //gst_editor_bin_debug_output(editor->canvas->selection);

  gst_editor_statusbar_message ("Doing debug output...");
  GstBin *top = GST_BIN (GST_EDITOR_ITEM(editor->canvas->bin)->object);
  elements = g_list_last (GST_BIN_CHILDREN (top));
  g_print("Getting Gstreamer Children, Total %d\n",
      g_list_length (GST_BIN_CHILDREN (top)));

  while (elements) {
    if (GST_IS_ELEMENT (elements->data)) {
      g_print ("Found Gstreamer Element:%s\n",
          GST_OBJECT_NAME (elements->data));
      for (srcpads = g_list_last (GST_ELEMENT (elements->data)->srcpads);
           srcpads != NULL;
           srcpads = g_list_previous (srcpads)) {
        /*
         * FIXME: This leaks buffer probes.
         * Use gst_pad_remove_probe().
         */
        gst_pad_add_probe (GST_PAD (srcpads->data),
            GST_PAD_PROBE_TYPE_BUFFER, cb_output, editor, NULL);
      }
    } else {
      g_print ("unknown child type \n");
    }
    elements = g_list_previous (elements);
  }
}

void
gst_editor_on_autosize (GtkCheckButton * autosize, GstEditor * editor)
{
    gboolean value;
    gdouble x, y,width,height;
    GooCanvasBounds bounds;
    value=gtk_toggle_button_get_active((GtkToggleButton*)autosize);
    editor->canvas->autosize=value;
    if (value){
	g_object_set(editor->canvas->bin, "width",editor->canvas->widthbackup,"height",editor->canvas->heightbackup,NULL);
        if (editor->canvas->bin) g_object_get (editor->canvas->bin, "width", &width, "height", &height, NULL);
	gtk_spin_button_set_value(editor->sw,width);
	gtk_spin_button_set_value(editor->sh,height);
        goo_canvas_item_get_bounds (GOO_CANVAS_ITEM (editor->canvas->bin), &bounds);
        x = bounds.x1;
        y = bounds.y1;
        goo_canvas_set_bounds (GOO_CANVAS (editor->canvas), x - 4, y - 4,
        x + width +3, y + height +3);
        //g_print("Gotten Object Size %d and %d size\n",(int)width,(int)height);     
	}
    else{
      //g_print("Will Show Adjustment Values According to bins size\n");     

    }
    //g_print("AutoSize toggled %d\n",(int)value);
}

void
gst_editor_on_spinbutton (GtkSpinButton * spinheight, GstEditor * editor)
{
  // g_print("Spinbutton!!!\n");
  gdouble x, y, width, height;
  GooCanvasBounds bounds;
  if (editor->canvas->autosize) {  // set value back
    if (editor->canvas->bin) {
      g_object_get (
          editor->canvas->bin, "width", &width, "height", &height, NULL);
      gtk_spin_button_set_value (editor->sw, width);
      gtk_spin_button_set_value (editor->sh, height);
      // g_print("Gotten Object Size %d and %d size\n",(int)width,(int)height);
    }
  } else {
    height = gtk_spin_button_get_value_as_int (editor->sh);
    width = gtk_spin_button_get_value_as_int (editor->sw);
    // g_print("Setting editor Canvas Value!!!\n");
    g_object_set (editor->canvas->bin, "width", width, "height", height, NULL);
    // g_print("FinishedSetting editor Canvas Value!!!\n");
    goo_canvas_item_get_bounds (GOO_CANVAS_ITEM (editor->canvas->bin), &bounds);
    x = bounds.x1;
    y = bounds.y1;
    goo_canvas_set_bounds (GOO_CANVAS (editor->canvas),
        x - 4, y - 4, x + width + 3, y + height + 3);
  }
}

/*  gboolean active;

  active = gtk_toggle_button_get_active (toggle);

  if (active) {
    gst_editor_statusbar_message ("Sorting bin...");

    g_timeout_add (200, (GSourceFunc) sort, editor);
  } else {
    gst_editor_statusbar_message ("Finished sorting.");
    g_source_remove_by_user_data (editor);
  }*/


static gint
on_delete_event (GtkWidget * widget, GdkEvent * event, GstEditor * editor)
{
  g_object_unref (G_OBJECT (editor));

  return FALSE;
}

static void
on_canvas_notify (GObject * object, GParamSpec * param, GstEditor * editor)
{
  GValue v = { 0, };
  gchar *status;

  if (g_ascii_strcasecmp (param->name, "properties-visible") == 0) {
    g_value_init (&v, param->value_type);
    g_object_get_property (object, param->name, &v);
    g_object_set_property (gtk_builder_get_object (editor->builder,
                "view-element-inspector"), "active", &v);
  } else if (g_ascii_strcasecmp (param->name, "palette-visible") == 0) {
    g_message ("canvas property notify");
    g_value_init (&v, param->value_type);
    g_object_get_property (object, param->name, &v);
    g_object_set_property (gtk_builder_get_object (editor->builder,
                "view-utility-palette"), "active", &v);
  } else if (g_ascii_strcasecmp (param->name, "status") == 0) {
    g_object_get (object, "status", &status, NULL);
    gst_editor_statusbar_message (status);
    g_free (status);
  }
}

static void
on_element_tree_select (GstElementBrowserElementTree * element_tree,
    gpointer user_data)
{
  GstElement *element, *selected_bin;
  GstElementFactory *selected_factory;
  GstEditor *editor = GST_EDITOR (user_data);
  GstState state;

  g_return_if_fail (editor->canvas != NULL);
  g_object_get (element_tree, "selected", &selected_factory, NULL);
  g_object_get (editor->canvas, "selection", &selected_bin, NULL);

  /* GstEditorCanvas::selection is actually a GstEditorElement, not a
     GstElement */
  if (selected_bin)
    selected_bin = GST_ELEMENT (GST_EDITOR_ITEM (selected_bin)->object);

  if (!selected_bin)
    selected_bin = GST_ELEMENT (GST_EDITOR_ITEM (editor->canvas->bin)->object);
  else if (!GST_IS_BIN (selected_bin))
    selected_bin = GST_ELEMENT (GST_OBJECT_PARENT (selected_bin));

  /* Check if we're allowed to add to the bin, ie if it's paused.
   * if not, throw up a warning */
  if (gst_element_get_state (selected_bin, &state, NULL,
          GST_CLOCK_TIME_NONE) != GST_STATE_CHANGE_SUCCESS ||
      state == GST_STATE_PLAYING) {
    gchar *message =
        g_strdup_printf ("bin %s is in PLAYING state, you cannot add "
        "elements to it in this state !",
        gst_element_get_name (selected_bin));

    //gst_editor_popup_warning (message);
    g_free (message);
    return;
  }

  //GtkWidget * check = lookup_widget(GTK_WIDGET(togglebutton), "defaultnamebutton");
  GtkWidget * check =
      GTK_WIDGET (gtk_builder_get_object (editor->builder, "defaultnamebutton"));
  GtkWidget * entry =
      GTK_WIDGET (gtk_builder_get_object (editor->builder, "elementname"));
  gboolean b_act;
  const gchar *elementname = gtk_entry_get_text (GTK_ENTRY (entry));
  g_object_get (GTK_BUTTON (check), "active", &b_act, NULL);
  if (b_act)
    element = gst_element_factory_create (selected_factory, elementname);
  else
    element = gst_element_factory_create (selected_factory, NULL);

  g_return_if_fail (element != NULL);
  gst_bin_add (GST_BIN (selected_bin), element);
}

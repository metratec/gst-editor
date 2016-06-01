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

#include "gsteditor.h"
#include "gsteditoritem.h"
#include "gsteditorelement.h"
#include "gsteditorstatusbar.h"
#include "stockicons.h"
#include <gst/common/gste-common.h>
#include <gst/common/gste-debug.h>
#include "../element-browser/browser.h"
#include <gst/element-browser/element-tree.h>
#include <goocanvas.h>
#include "../../../pixmaps/pixmaps.h"



#define GST_CAT_DEFAULT gste_debug_cat

static void gst_editor_class_init (GstEditorClass * klass);
static void gst_editor_init (GstEditor * project_editor);
static void gst_editor_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_editor_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_editor_dispose (GObject * object);
static void gst_editor_connect_func (const gchar * handler_name,
    GObject * object,
    const gchar * signal_name,
    const gchar * signal_data,
    GObject * connect_object, gboolean after, gpointer user_data);

static gint on_delete_event (GtkWidget * widget,
    GdkEvent * event, GstEditor * editor);
static void on_canvas_notify (GObject * object,
    GParamSpec * param, GstEditor * ui);
static void on_xml_loaded (GstXML * xml, GstObject * object, xmlNodePtr self,
    GData ** datalistp);
static void
on_element_tree_select (GstElementBrowserElementTree * element_tree,
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

/* GST_EDITOR_ERROR quark - FIXME: maybe move this to a more generic part */
#define GST_EDITOR_ERROR	(gst_editor_error_quark ())

GQuark
gst_editor_error_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string ("gst-editor-error-quark");
  return quark;
}

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
  GList *list;
  GdkPixbuf *pixbuf;
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_ref (G_TYPE_OBJECT);

  gobject_class->get_property = gst_editor_get_property;
  gobject_class->set_property = gst_editor_set_property;
  gobject_class->dispose = gst_editor_dispose;

  g_object_class_install_property (gobject_class, PROP_FILENAME,
      g_param_spec_string ("filename", "Filename", "File name",
          NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  _gst_editor_stock_icons_init ();

  /* give all windows the same icon (in theory) */
  pixbuf = gdk_pixbuf_new_from_inline (-1, gst_editor_stock_image, FALSE, NULL);
  list = g_list_prepend (NULL, pixbuf);
  gtk_window_set_default_icon_list (list);

  g_list_free (list);
  g_object_unref (G_OBJECT (pixbuf));
}

static void
gst_editor_init (GstEditor * editor)
{
  connect_struct data;
  GModule *symbols;
  gchar *path;

  symbols = g_module_open (NULL, 0);

  data.editor = editor;
  data.symbols = symbols;

  path = gste_get_ui_file ("editor.glade2");
  if (!path)
    g_error ("GStreamer Editor user interface file 'editor.glade2' not found.");
  editor->xml = glade_xml_new (path, "main_project_window", NULL);

  if (!editor->xml) {
    g_error ("GStreamer Editor could not load main_project_window from %s",
        path);
  }
  g_free (path);

  glade_xml_signal_autoconnect_full (editor->xml, gst_editor_connect_func,
      &data);

  editor->window = glade_xml_get_widget (editor->xml, "main_project_window");

  editor->sw=glade_xml_get_widget (editor->xml, "spinbutton1");
  editor->sh=glade_xml_get_widget (editor->xml, "spinbutton2");

  gtk_widget_show (editor->window);
//Code for element tree
  editor->element_tree =
      g_object_new (gst_element_browser_element_tree_get_type (), NULL);
  gtk_box_pack_start (GTK_BOX (glade_xml_get_widget (editor->xml,
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

  gtk_container_add (GTK_CONTAINER (glade_xml_get_widget (editor->xml,
              "canvasSW")), GTK_WIDGET (editor->canvas));

  _num_editor_windows++;

  gst_editor_statusbar_init (editor);

  g_signal_connect (editor->window, "delete-event",
      G_CALLBACK (on_delete_event), editor);
  g_signal_connect (editor->canvas, "notify", G_CALLBACK (on_canvas_notify),
      editor);
  editor->outputmutex=g_mutex_new();
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
        editor->filename = g_strdup_printf ("untitled-%d.xml", count++);
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
}

/*
 * helper functions
 */

/* show an error dialog with a gerror and debug string */
static void
gst_editor_dialog_gerror (GtkWindow * window, GstMessage* message)
{
  GtkWidget *error_dialog;
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
      GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, err->message);
  /* add a stop pipeline button */
//  gtk_dialog_add_button (GTK_DIALOG (error_dialog), "STOP",
//      GTK_RESPONSE_CANCEL);
  gtk_dialog_add_button (GTK_DIALOG (error_dialog), GTK_STOCK_CANCEL,
      GTK_RESPONSE_CANCEL);
  /* add a debug button if there is debug info */
  if (debug) {
    gtk_dialog_add_button (GTK_DIALOG (error_dialog), GTK_STOCK_DIALOG_INFO,
        GST_EDITOR_RESPONSE_DEBUG);
  }
  /* make sure OK is on the right and default action to be HIG-compliant */
  gtk_dialog_add_button (GTK_DIALOG (error_dialog), GTK_STOCK_OK,
      GTK_RESPONSE_OK);



  gtk_dialog_set_default_response (GTK_DIALOG (error_dialog), GTK_RESPONSE_OK);
  response = gtk_dialog_run (GTK_DIALOG (error_dialog));

  if (response == GTK_RESPONSE_CANCEL) {
      GstObject *obj = GST_OBJECT (GST_MESSAGE_SRC (message));
      if (GST_IS_ELEMENT (obj)){
	GstElement* top=obj;
	while (GST_ELEMENT_PARENT(top)) top=GST_ELEMENT_PARENT(top);
	//g_print("GstEditor: stopping on demand\n");
        gst_element_set_state(top,GST_STATE_NULL);
        GstEditorItem *item = gst_editor_item_get (top);
        gst_editor_element_stop_child(GST_EDITOR_ELEMENT(item));
        //g_idle_add ((GSourceFunc) gst_editor_element_sync_state,
        //    item /*editor_element */ );
      }
  }
  else if (response == GST_EDITOR_RESPONSE_DEBUG) {
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

#if 0
/* show an error dialog given just an error string (for internal use) */
static void
gst_editor_dialog_error (GtkWindow * window, const gchar * message)
{
  GError *error;

  error = g_error_new (GST_EDITOR_ERROR, 0, message);
  gst_editor_dialog_gerror (window, error, NULL);
  g_error_free (error);
}
#endif

/* GStreamer callbacks */
static void
gst_editor_pipeline_deep_notify (GstObject * object, GstObject * orig,
    GParamSpec * pspec, gchar ** excluded_props)
{
}

#ifdef SIGNAL_ERROR             // gstreamer < 0.10
static void
gst_editor_pipeline_error (GObject * object, GstObject * source,
    GError * error, gchar * debug, GstEditor * editor)
{
  gchar *name = gst_object_get_path_string (source);
  GError *my_error;

  my_error = g_error_copy (error);
  g_free (my_error->message);
  my_error->message = g_strdup_printf ("%s: %s", name, error->message);

  gst_editor_dialog_gerror (GTK_WINDOW (editor->window), my_error, debug);
  g_error_free (my_error);
  g_free (name);
}
#else
static gboolean
gst_editor_pipeline_message (GstBus * bus, GstMessage * message, gpointer data)
{
  GstEditor *editor = GST_EDITOR (data);

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
    {
      gst_editor_dialog_gerror (GTK_WINDOW (editor->window),message);
      break;
    }
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
      if (GST_IS_ELEMENT (obj)){ 
	gst_element_set_state((GstElement*)obj,GST_STATE_NULL);
        GstEditorItem *item = gst_editor_item_get (obj);
	//g_print("adding idle function in main bus watcher");
        gst_editor_element_stop_child(GST_EDITOR_ELEMENT(item));
        g_idle_add ((GSourceFunc) gst_editor_element_sync_state,
            item /*editor_element */ );
      }
    }
    default:
      /* unhandled message */
      break;
  }
  /* remove message from the queue */
  return TRUE;
}
#endif

/* connect useful GStreamer signals to pipeline */
static void
gst_editor_element_connect (GstEditor * editor, GstElement * pipeline)
{
  GstBus *bus;

  g_signal_connect (pipeline, "deep_notify",
      G_CALLBACK (gst_editor_pipeline_deep_notify), editor);
#ifdef SIGNAL_ERROR             // gstreamer < 0.10
  g_signal_connect (pipeline, "error",
      G_CALLBACK (gst_editor_pipeline_error), editor);
#else
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, &gst_editor_pipeline_message, editor);
#endif
}

/* we need more control here so... */
static void
gst_editor_connect_func (const gchar * handler_name,
    GObject * object,
    const gchar * signal_name,
    const gchar * signal_data,
    GObject * connect_object, gboolean after, gpointer user_data)
{
  gpointer func_ptr;
  connect_struct *data = (connect_struct *) user_data;

  if (!g_module_symbol (data->symbols, handler_name, &func_ptr))
    g_warning ("GstEditor: could not find signal handler '%s'.", handler_name);
  else {
    GCallback func;

    func = *(GCallback) (func_ptr);
    if (after)
      g_signal_connect_after (object, signal_name, func,
          (gpointer) data->editor);
    else
      g_signal_connect (object, signal_name, func, (gpointer) data->editor);
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

typedef struct
{
  GtkWidget *selection;
  GstEditor *editor;
}
file_select;

static gboolean
do_save (GstEditor * editor)
{
  GstElement *element;

#if 1
  FILE *file;

  if (!(file = fopen (editor->filename, "w"))) {
    g_warning ("%s could not be saved...", editor->filename);
    return FALSE;
  }
#endif
  g_object_get (editor->canvas, "bin", &element, NULL);
#if 1
  if (gst_xml_write_file (element, file) < 0)
    g_warning ("error saving xml");
  fclose (file);
#else
  if (gst_xml_write_filename (element, editor->filename) < 0)
    g_warning ("error saving xml");
#endif
  gst_editor_statusbar_message ("Pipeline saved to %s.", editor->filename);

  return TRUE;
}

static void
on_save_as_file_selected (GtkWidget * button, file_select * data)
{
  GtkWidget *selector = data->selection;
  GstEditor *editor = data->editor;

  g_object_set (editor, "filename",
      gtk_file_selection_get_filename (GTK_FILE_SELECTION (selector)), NULL);

  do_save (editor);

  g_free (data);
}

void
gst_editor_on_save_as (GtkWidget * widget, GstEditor * editor)
{
  GtkWidget *file_selector;
  file_select *file_data = g_new0 (file_select, 1);

  file_selector = gtk_file_selection_new ("Please select a file for saving.");

  g_object_set (file_selector, "filename", editor->filename, NULL);

  file_data->selection = file_selector;
  file_data->editor = editor;

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (file_selector)->
          ok_button), "clicked", GTK_SIGNAL_FUNC (on_save_as_file_selected),
      file_data);

  /* Ensure that the dialog box is destroyed when the user clicks a button. */
  g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (file_selector)->
          ok_button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
      (gpointer) file_selector);
  g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (file_selector)->
          cancel_button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
      (gpointer) file_selector);

  /* Display that dialog */
  gtk_widget_show (file_selector);
}

void
gst_editor_on_save (GtkWidget * widget, GstEditor * editor)
{
  if (editor->need_name) {
    gst_editor_on_save_as (NULL, editor);
    return;
  }

  do_save (editor);
}

static void
on_load_file_selected (GtkWidget * button, file_select * data)
{
  GtkWidget *selector = data->selection;
  const gchar *file_name;
  GstEditor *editor = data->editor;

  file_name = gtk_file_selection_get_filename (GTK_FILE_SELECTION (selector));

  gst_editor_load (editor, file_name);

  g_free (data);
}

void
gst_editor_load (GstEditor * editor, const gchar * file_name)
{
  gboolean err;
  GList *l;
  GstElement *pipeline;
  GstXML *xml;
  GstEditorItemAttr *attr = NULL;
  gdouble x,y,width,height;//just for calculating some things
  GooCanvasBounds bounds;
  gulong handler;//to remove handler afterwards, fixes bug in GStreamer
  xml = gst_xml_new ();

  /* create a datalist to retrieve extra information from xml */
  g_datalist_init (&editor->attributes);

  handler=g_signal_connect (G_OBJECT (xml), "object_loaded",
      G_CALLBACK (on_xml_loaded), &editor->attributes);
//first unref all the old stuff
  if ((editor->canvas)&&(editor->canvas->bin)&&(GST_EDITOR_ITEM(editor->canvas->bin)->object)){
    g_object_unref(G_OBJECT(GST_EDITOR_ITEM(editor->canvas->bin)->object));
  }

  err = gst_xml_parse_file (xml, (xmlChar *) file_name, NULL);

  g_signal_handler_disconnect         (G_OBJECT (xml),handler);

  if (err != TRUE) {
    g_warning ("parse of xml file '%s' failed", file_name);
    return;
  }

  l = gst_xml_get_topelements (xml);
  if (!l) {
    g_warning ("no toplevel pipeline element in file '%s'", file_name);
    return;
  }

  if (l->next)
    g_warning ("only one toplevel element is supported at this time");

  pipeline = GST_ELEMENT (l->data);

  /* FIXME: through object properties ? */
  EDITOR_DEBUG ("loaded: attributes: %p", editor->attributes);
  /* FIXME: attributes needs to come first ! */
  g_object_set (editor->canvas, "attributes", &editor->attributes, "bin", pipeline, NULL);
  g_object_set (editor, "filename", file_name, NULL);


  attr = g_datalist_get_data (editor->attributes, GST_ELEMENT_NAME(pipeline));
  /* now decide */
  if (attr) {
    editor->canvas->widthbackup= attr->w;
    editor->canvas->heightbackup= attr->h;
    g_object_set(editor->canvas->bin, "width",editor->canvas->widthbackup,"height",editor->canvas->heightbackup,NULL);
    if (editor->canvas->bin) g_object_get (editor->canvas->bin, "width", &width, "height", &height, NULL);
    goo_canvas_item_get_bounds (GOO_CANVAS_ITEM (editor->canvas->bin), &bounds);
    x = bounds.x1;
    y = bounds.y1;
    goo_canvas_set_bounds (GOO_CANVAS (editor->canvas), x - 4, y - 4,
        x + width +3, y + height +3);
  }
  else g_print("Element attributes for %s not found!\n",GST_ELEMENT_NAME(pipeline));
  gst_editor_statusbar_message ("Pipeline loaded from %s.", editor->filename);
  gst_editor_element_connect (editor, pipeline);
}

void
gst_editor_on_open (GtkWidget * widget, GstEditor * editor)
{
  GtkWidget *file_selector;
  file_select *file_data = g_new0 (file_select, 1);

  file_selector = gtk_file_selection_new ("Please select a file to load.");

  file_data->selection = file_selector;
  file_data->editor = editor;

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
      "clicked", G_CALLBACK (on_load_file_selected), file_data);

  /* Ensure that the dialog box is destroyed when the user clicks a button. */
  g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (file_selector)->
          ok_button), "clicked", G_CALLBACK (gtk_widget_destroy),
      (gpointer) file_selector);
  g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (file_selector)->
          cancel_button), "clicked", G_CALLBACK (gtk_widget_destroy),
      (gpointer) file_selector);

  /* Display that dialog */
  gtk_widget_show (file_selector);
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
    g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
        G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (dialog));

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
//    request = gnome_request_dialog (FALSE,
//        "Please enter in a pipeline description. "
//        "See the gst-launch man page for help on the syntax.",
//        "fakesrc ! fakesink",
//        0, have_pipeline_description, editor, GTK_WINDOW (editor->window));
//    gnome_dialog_close_hides (GNOME_DIALOG (request), TRUE);
    request = gtk_dialog_new_with_buttons ("Gst-Editor",
        GTK_WINDOW (editor->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_STOCK_OK,
        GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    label = gtk_label_new ("Please enter in a pipeline description. "
        "See the gst-launch man page for help on the syntax.");
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (request)->vbox), label);
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
//  gnome_help_display ("gst-editor-manual", NULL, NULL);
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
  GtkWidget *about;
  GdkPixbuf *pixbuf;
  const gchar *authors[] = {  "Hannes Bistry","Andy Wingo", "Erik Walthinsen",
    "Jan Schmidt", NULL
  };

  about = g_object_new (GTK_TYPE_ABOUT_DIALOG,
      "name", "GStreamer Pipeline Editor",
      "version", VERSION,
      "copyright", "(c) 2001-2007 GStreamer Team, 2008-2010 Hannes Bistry",
      "comments", "A graphical pipeline editor for "
      "GStreamer capable of loading and saving XML.",
      "website", "http://gstreamer.freedesktop.org/", "authors", authors, NULL);

  pixbuf = gtk_widget_render_icon (about, GST_EDITOR_STOCK_LOGO,
      GTK_ICON_SIZE_DIALOG, NULL);
  if (!pixbuf)
    g_warning ("no pixbuf found");
  else
    g_object_set (about, "logo", pixbuf, NULL);

  g_signal_connect (about, "response", G_CALLBACK (gtk_widget_destroy), about);

  gtk_widget_show (about);
}

static gboolean
sort (GstEditor * editor)
{
  gst_editor_statusbar_message ("Change in bin pressure: %f",
      gst_editor_bin_sort (editor->canvas->bin, 0.1));
  return TRUE;
}
//callback that generates an output message to stderr
static gboolean
cb_output (GstPad    *pad,
	      GstBuffer *buffer,
	      gpointer   u_data)
{
  
  GstEditor * editor=GST_EDITOR(u_data);
  guint64 time;
  //fprintf(stderr,"Buffer:)");
  struct timeval now;
  gettimeofday(&now, NULL);
  time=((now.tv_sec*1000000000ULL)+(now.tv_usec*1000ULL));
  g_mutex_lock(editor->outputmutex);
  if (GST_STATE(GST_OBJECT(pad)->parent)==GST_STATE_PLAYING){
    fprintf(stderr,"%s %llu %llu %u %llu %ld %d %lu %s \n",(GST_OBJECT(pad)->parent->name),buffer->timestamp,time, buffer->size, buffer->offset, clock(),getpid(),pthread_self(),(GST_OBJECT(pad->peer)->parent->name));//Element Name, timestamp, timenow, jiffies now,Prozessnr, threadnr,Peer Element Name
  }
  g_mutex_unlock(editor->outputmutex);  
  return TRUE;
}


void
gst_editor_on_sort_toggled (GtkToggleButton * toggle, GstEditor * editor)
{
  GList *elements,*srcpads;
    
    //gst_editor_statusbar_message ("Doing debug output...");
    //gst_editor_bin_debug_output(editor->canvas->selection);

  gst_editor_statusbar_message ("Doing debug output...");
  GstBin *top=(GstBin *)(GST_EDITOR_ITEM(editor->canvas->bin)->object);
  elements = g_list_last(GST_BIN_CHILDREN (top));
  g_print("Getting Gstreamer Children, Total %d\n", g_list_length(GST_BIN_CHILDREN (GST_BIN (top))));
  while (elements) {
    if (GST_IS_ELEMENT(elements->data)){
      g_print("Found Gstreamer Element:%s\n",GST_OBJECT_NAME(elements->data) );
      srcpads=g_list_last(GST_ELEMENT(elements->data)->srcpads);
      while (srcpads) {
	if (42) gst_pad_add_buffer_probe ((GstPad*)srcpads->data, G_CALLBACK (cb_output), (gpointer)editor);
	//else  g_print("Removing buffer probes not implemented:)");//gst_pad_remove_buffer_probe (pad, G_CALLBACK (cb_output), NULL);
	srcpads = g_list_previous (srcpads);
      }
    }
    else g_print("unknown child type \n");
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
gst_editor_on_spinbutton(GtkSpinButton * spinheight, GstEditor * editor)
{
    //g_print("Spinbutton!!!\n");     
    gdouble x, y, width, height;
    GooCanvasBounds bounds;
    if (editor->canvas->autosize){//set value back
        if (editor->canvas->bin) g_object_get (editor->canvas->bin, "width", &width, "height", &height, NULL);
	gtk_spin_button_set_value(editor->sw,width);
	gtk_spin_button_set_value(editor->sh,height);
        //g_print("Gotten Object Size %d and %d size\n",(int)width,(int)height);    
    }
    else{
      int value;
      height=gtk_spin_button_get_value_as_int(editor->sh);
      width=gtk_spin_button_get_value_as_int(editor->sw);
      //g_print("Setting editor Canvas Value!!!\n");   
      g_object_set(editor->canvas->bin, "width",width,"height",height,NULL);
      //g_print("FinishedSetting editor Canvas Value!!!\n");
      goo_canvas_item_get_bounds (GOO_CANVAS_ITEM (editor->canvas->bin), &bounds);
       x = bounds.x1;
       y = bounds.y1;
       goo_canvas_set_bounds (GOO_CANVAS (editor->canvas), x - 4, y - 4,
       x + width +3, y + height +3);

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
    g_object_set_property (G_OBJECT (glade_xml_get_widget (editor->xml,
                "view-element-inspector")), "active", &v);
  } else if (g_ascii_strcasecmp (param->name, "palette-visible") == 0) {
    g_message ("canvas property notify");
    g_value_init (&v, param->value_type);
    g_object_get_property (object, param->name, &v);
    g_object_set_property (G_OBJECT (glade_xml_get_widget (editor->xml,
                "view-utility-palette")), "active", &v);
  } else if (g_ascii_strcasecmp (param->name, "status") == 0) {
    g_object_get (object, "status", &status, NULL);
    gst_editor_statusbar_message (status);
    g_free (status);
  }
}

static void
on_xml_loaded (GstXML * xml, GstObject * object, xmlNodePtr self,
    GData ** datalistp)
{
  xmlNodePtr children = self->xmlChildrenNode;
  GstEditorItemAttr *attr = NULL;       /* GUI attributes in editor canvas */

  attr = g_malloc (sizeof (GstEditorItemAttr));
  //g_print("xml for object %s with pointer %p loaded, getting attrs",GST_OBJECT_NAME(object),(gpointer)object);
  GST_DEBUG_OBJECT (object, "xml for object %s with pointer %p loaded, getting attrs",GST_OBJECT_NAME(object),(gpointer)object);
  while (children) {
    if (!g_ascii_strcasecmp ((gchar *) children->name, "item")) {
      xmlNodePtr nodes = children->xmlChildrenNode;

      while (nodes) {
        if (!g_ascii_strcasecmp ((gchar *) nodes->name, "x")) {
          attr->x = g_ascii_strtod ((gchar *) xmlNodeGetContent (nodes), NULL);
        } else if (!g_ascii_strcasecmp ((gchar *) nodes->name, "y")) {
          attr->y = g_ascii_strtod ((gchar *) xmlNodeGetContent (nodes), NULL);
        } else if (!g_ascii_strcasecmp ((gchar *) nodes->name, "w")) {
          attr->w = g_ascii_strtod ((gchar *) xmlNodeGetContent (nodes), NULL);
        } else if (!g_ascii_strcasecmp ((gchar *) nodes->name, "h")) {
          attr->h = g_ascii_strtod ((gchar *) xmlNodeGetContent (nodes), NULL);
        }
        nodes = nodes->next;
      }
      GST_DEBUG_OBJECT (object, "loaded with x: %f, y: %f, w: %f, h: %f",
          attr->x, attr->y, attr->w, attr->h);
    }
    children = children->next;
  }

  /* save this in the datalist with the object's name as key */
  GST_DEBUG_OBJECT (object, "adding to datalistp %p", datalistp);
  g_datalist_set_data (datalistp, GST_OBJECT_NAME (object), attr);
}

static void
on_element_tree_select (GstElementBrowserElementTree * element_tree,
    gpointer user_data)
{
  GstElement *element, *selected_bin;
  GstElementFactory *selected_factory;
  GstEditor *editor = GST_EDITOR (user_data);

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
  if (gst_element_get_state (selected_bin, NULL, NULL,
          GST_CLOCK_TIME_NONE) == GST_STATE_PLAYING) {
    gchar *message =
        g_strdup_printf ("bin %s is in PLAYING state, you cannot add "
        "elements to it in this state !",
        gst_element_get_name (selected_bin));

    //gst_editor_popup_warning (message);
    g_free (message);
    return;
  }

  //GtkWidget * check = lookup_widget(GTK_WIDGET(togglebutton), "defaultnamebutton");
  GtkWidget * check = glade_xml_get_widget(editor->xml,"defaultnamebutton");
  GtkWidget * entry = glade_xml_get_widget(editor->xml,"elementname");
  gboolean b_act;
  char* elementname;
  elementname=gtk_entry_get_text((GtkEntry*)entry);
  g_object_get(GTK_BUTTON(check), "active", &b_act, NULL);
  if (b_act) element = gst_element_factory_create(selected_factory,elementname);
  else element = gst_element_factory_create (selected_factory, NULL);


  g_return_if_fail (element != NULL);
  gst_bin_add (GST_BIN (selected_bin), element);
}

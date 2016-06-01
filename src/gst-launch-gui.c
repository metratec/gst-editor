/* -*- Mode: C; c-basic-offset: 8; indent-tabs-mode: t -*- */
/* gst-launch-gui: GStreamer Graphical Launcher
 * Copyright (C) <2001,2002> Steve Baker, Andy Wingo
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#ifdef HAVE_MMAP_H
#include <sys/mman.h>
#endif
#include <sys/stat.h>

#include <fcntl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <gst/gst.h>

#include <gst/editor/editor.h>
#include <gst/element-ui/gst-element-ui.h>
#include <gst/debug-ui/debug-ui.h>

GtkWidget *start_but, *pause_but, *parse_but, *status;
GtkWidget *window;
GtkWidget *element_ui;
GtkWidget *prop_box, *dparam_box;
GstElement *pipeline;
GtkTreeStore *store = NULL;
GtkTreeView *view = NULL;

typedef void (*found_handler) (GstElement * element, gint xid, void *priv);

static gint
quit_live (GtkWidget * window, GdkEventAny * e, gpointer data)
{
  gtk_main_quit ();
  return FALSE;
}

gboolean
idle_func (gpointer data)
{
  //return gst_bin_iterate (GST_BIN (pipeline));
  return TRUE;
}

void
load_history (GtkWidget * pipe_combo)
{

  gchar *history_filename, *history_string, **split, **walk;
  gint num_entries = 0, entries_limit = 50;
  GList *history = NULL;
  FILE *history_file;

  history_filename =
      g_strdup_printf ("%s/.gstreamer-guilaunch.history", g_get_home_dir ());

  if (!g_file_get_contents (history_filename, &history_string, NULL, NULL)) {
    history_string = g_strdup ("");
  }

  split = g_strsplit (history_string, "\n", 0);

  /* go to the end of the list and append entries from there */
  walk = split;
  while (*walk)
    walk++;
  while (--walk >= split && num_entries++ < entries_limit)
    history = g_list_append (history, *walk);

  gtk_combo_set_popdown_strings (GTK_COMBO (pipe_combo), history);

  history_file = fopen (history_filename, "a");
  if (history_file == NULL) {
    perror ("couldn't open history file");
  }
  g_object_set_data (G_OBJECT (pipe_combo), "history", history);
  g_object_set_data (G_OBJECT (pipe_combo), "history_file", history_file);
}

void
build_debug_page (GtkWidget * notebook)
{
#ifndef _MSC_VER
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), gst_debug_ui_new (),
      gtk_label_new ("Debug"));
  
#endif
}

void
handle_have_size (GstElement * element, int width, int height)
{
  g_print ("setting window size\n");
  gtk_widget_set_usize (GTK_WIDGET (g_object_get_data (G_OBJECT (element),
              "gtk_socket")), width, height);
  gtk_widget_show_all (GTK_WIDGET (g_object_get_data (G_OBJECT (element),
              "vid_window")));
}

void
xid_handler (GstElement * element, gint xid, void *priv)
{
  GtkWidget *gtk_socket, *vid_window;

  g_print ("handling xid %d\n", xid);
  vid_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_socket = gtk_socket_new ();
  gtk_widget_show (gtk_socket);

  gtk_container_add (GTK_CONTAINER (vid_window), gtk_socket);

  gtk_widget_realize (gtk_socket);
  gtk_socket_steal (GTK_SOCKET (gtk_socket), xid);

  gtk_object_set (GTK_OBJECT (vid_window), "allow_grow", TRUE, NULL);
  gtk_object_set (GTK_OBJECT (vid_window), "allow_shrink", TRUE, NULL);

  g_signal_connect (G_OBJECT (element), "have_size",
      G_CALLBACK (handle_have_size), element);
  g_object_set_data (G_OBJECT (element), "vid_window", vid_window);
  g_object_set_data (G_OBJECT (element), "gtk_socket", gtk_socket);
}

void
selection_changed_cb (GtkTreeView * treeview, GtkMovementStep unused1,
    gint unused2, GstElement * unused3)
{
  GstElement *element;
  GtkTreeIter iter;
  GtkTreeModel *model = GTK_TREE_MODEL (store);

  if (!gtk_tree_selection_get_selected (gtk_tree_view_get_selection (view),
          &model, &iter))
    return;

  gtk_tree_model_get (model, &iter, 1, &element, -1);

  g_object_set (G_OBJECT (element_ui), "element", element, NULL);
}

void
build_tree (GtkTreeIter * parent, GstBin * bin)
{
  //const GList *children;
  gboolean done = FALSE;
  GtkWidget *tree, *item;
  GtkTreeIter *iter;
  GstIterator *children;


  //children = gst_bin_get_list (bin);
  children = gst_bin_iterate_elements (bin);

  iter = g_new0 (GtkTreeIter, 1);
  while (!done)
     {
    GstElement * child;
    switch (gst_iterator_next (children, &child))
       {
      case GST_ITERATOR_OK:
        gtk_tree_store_append (store, iter, parent);
        gtk_tree_store_set (store, iter, 0,
            gst_object_get_name (GST_OBJECT (&child)), 1, &child, -1);
        if (GST_IS_BIN (&child))
          build_tree (iter, GST_BIN (&child));
        break;
      case GST_ITERATOR_RESYNC:
        
            //...rollback changes to items...
            gst_iterator_resync (children);
        break;
      case GST_ITERATOR_ERROR:
        
            //...wrong parameter were given...
      case GST_ITERATOR_DONE:
      default:
        done = TRUE;
        break;
      }
    }
  g_free (iter);
  gst_iterator_free (children);
  
/*
  //while (children) {
  while (gst_iterator_next(children, &child) ==  GST_ITERATOR_OK) {
	 //GstElement *child;

	 //child = GST_ELEMENT (children->data);
	 //children = g_list_next (children);

	 gtk_tree_store_append (store, iter, parent);
	 gtk_tree_store_set (store, iter, 0,
		  gst_object_get_name (GST_OBJECT (&child)), 1, &child, -1);

	 if (GST_IS_BIN (&child))
		build_tree (iter, GST_BIN (&child));
  }

  g_free (iter);
  */
}

void
parse_callback (GtkWidget * widget, GtkWidget * pipe_combo)
{
  GError *err = NULL;

  GList *history =
      (GList *) g_object_get_data (G_OBJECT (pipe_combo), "history");
  FILE *history_file =
      (FILE *) g_object_get_data (G_OBJECT (pipe_combo), "history_file");
  gchar *last_pipe =
      (gchar *) g_object_get_data (G_OBJECT (widget), "last_pipe");
  gchar *try_pipe =
      g_strdup (gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (pipe_combo)->entry)));

  if (pipeline) {
    g_print ("unreffing\n");
    gst_object_unref (GST_OBJECT (pipeline));
    pipeline = NULL;
  }
  g_print ("trying pipeline: %s\n", try_pipe);

  pipeline = (GstElement *) gst_parse_launch (try_pipe, &err);

  if (!pipeline) {
    gtk_label_set_text (GTK_LABEL (status),
        err ? err->message : "unknown parse error");
    return;
  }

  g_object_set (G_OBJECT (element_ui), "element", NULL, NULL);

  gtk_widget_set_sensitive (GTK_WIDGET (start_but), TRUE);

  gtk_tree_store_clear (store);
  build_tree (NULL, GST_BIN (pipeline));
  gtk_tree_view_expand_all (view);

  if (last_pipe == NULL || strcmp (last_pipe, try_pipe) != 0) {
    gchar *write_pipe = g_strdup_printf ("%s\n", try_pipe);

    g_object_set_data (G_OBJECT (widget), "last_pipe", try_pipe);
    fwrite (write_pipe, sizeof (gchar), strlen (write_pipe), history_file);
    fflush (history_file);
    history = g_list_prepend (history, try_pipe);
    gtk_combo_set_popdown_strings (GTK_COMBO (pipe_combo), history);
    g_free (write_pipe);
    g_free (last_pipe);
  }

}

void
start_callback (GtkWidget * widget, gpointer data)
{
  GtkWidget *pipe_combo =
      gtk_object_get_data (GTK_OBJECT (widget), "pipe_combo");

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
    gtk_widget_set_sensitive (GTK_WIDGET (pause_but), TRUE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pause_but), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (parse_but), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (pipe_combo), FALSE);
    gtk_label_set_text (GTK_LABEL (status), "playing");

    gst_element_set_state (pipeline, GST_STATE_PLAYING);

    /*if (!GST_FLAG_IS_SET (pipeline, GST_BIN_SELF_SCHEDULABLE))
       g_idle_add (idle_func, pipeline); */
  } else {
    gtk_widget_set_sensitive (GTK_WIDGET (pause_but), FALSE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pause_but), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (parse_but), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (pipe_combo), TRUE);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gtk_label_set_text (GTK_LABEL (status), "stopped");

    /*if (!GST_FLAG_IS_SET (pipeline, GST_BIN_SELF_SCHEDULABLE))
       g_idle_remove_by_data (pipeline); */
  }
}

void
pause_callback (GtkWidget * widget, gpointer data)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
    gst_element_set_state (pipeline, GST_STATE_PAUSED);
    gtk_label_set_text (GTK_LABEL (status), "paused");
  } else {
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    gtk_label_set_text (GTK_LABEL (status), "playing");
  }
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *parse_line, *pipe_combo, *notebook, *pane;
  GtkWidget *page_scroll;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  gchar **argvn, *joined;
  GdkPixbuf *icon = NULL;

  gst_init (&argc, &argv);
  gtk_init (&argc, &argv);
  gste_init ();

                  /***** set up the GUI *****/
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "delete-event", GTK_SIGNAL_FUNC (quit_live), NULL);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_window_set_default_size (GTK_WINDOW (window), 500, 300);

  icon = gdk_pixbuf_new_from_file (PIXMAP_DIR "gst-launch.png", NULL);
  if (icon) {
    gtk_window_set_icon (GTK_WINDOW (window), icon);
    g_object_unref (G_OBJECT (icon));
  } else {
    g_warning ("Icon file %s not found", PIXMAP_DIR "gst-launch.png");
  }

  parse_line = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), parse_line, FALSE, FALSE, 0);

  pipe_combo = gtk_combo_new ();
  gtk_combo_set_value_in_list (GTK_COMBO (pipe_combo), FALSE, FALSE);
  load_history (pipe_combo);

  parse_but = gtk_button_new_with_label ("Parse");
  gtk_box_pack_start (GTK_BOX (parse_line), pipe_combo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (parse_line), parse_but, FALSE, FALSE, 0);

  start_but = gtk_toggle_button_new_with_label ("Play");
  pause_but = gtk_toggle_button_new_with_label ("Pause");

  gtk_box_pack_start (GTK_BOX (parse_line), start_but, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (parse_line), pause_but, FALSE, FALSE, 0);

  gtk_widget_set_sensitive (GTK_WIDGET (start_but), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (pause_but), FALSE);

  gtk_signal_connect (GTK_OBJECT (start_but), "clicked",
      GTK_SIGNAL_FUNC (start_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (pause_but), "clicked",
      GTK_SIGNAL_FUNC (pause_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (parse_but), "clicked",
      GTK_SIGNAL_FUNC (parse_callback), pipe_combo);


  gtk_object_set_data (GTK_OBJECT (start_but), "pipe_combo", pipe_combo);

  store = gtk_tree_store_new (2, G_TYPE_STRING, GST_TYPE_ELEMENT);
  view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (store)));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (selection), "changed",
      G_CALLBACK (selection_changed_cb), NULL);
  gtk_tree_view_set_headers_visible (view, FALSE);
  gtk_tree_view_insert_column_with_attributes (view,
      -1, "Mix", gtk_cell_renderer_text_new (), "text", 0, NULL);
  column = gtk_tree_view_get_column (view, 0);
  gtk_tree_view_column_set_clickable (column, TRUE);

  prop_box = gtk_vbox_new (FALSE, 0);

  /* dparam_box = gtk_vbox_new(FALSE, 0); */

  notebook = gtk_notebook_new ();

  page_scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (page_scroll),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (page_scroll),
      prop_box);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_scroll,
      gtk_label_new ("Properties"));

/*
	page_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (page_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (page_scroll), dparam_box);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_scroll, gtk_label_new ("Dynamic Params"));
*/ 
#ifndef _MSC_VER
      build_debug_page (notebook);
#endif
  page_scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (page_scroll),
      GTK_WIDGET (view));
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (page_scroll),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  pane = gtk_hpaned_new ();
  gtk_paned_pack1 (GTK_PANED (pane), page_scroll, TRUE, TRUE);
  gtk_paned_pack2 (GTK_PANED (pane), notebook, TRUE, TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), pane, TRUE, TRUE, 0);

  status = gtk_label_new ("stopped");
  gtk_box_pack_start (GTK_BOX (vbox), status, FALSE, FALSE, 0);

  gtk_widget_show_all (window);

  element_ui = g_object_new (gst_element_ui_get_type (), "view-mode",
      GST_ELEMENT_UI_VIEW_MODE_FULL, NULL);
  gtk_box_pack_start (GTK_BOX (prop_box), element_ui, FALSE, FALSE, 0);

  gtk_widget_show (element_ui);

  if (argc) {
    argvn = g_new0 (char *, argc);
    memcpy (argvn, argv + 1, sizeof (char *) * (argc - 1));
    joined = g_strjoinv (" ", argvn);
    g_object_set (GTK_COMBO (pipe_combo)->entry, "text", joined, NULL);
    parse_callback (parse_but, pipe_combo);
  }

  gtk_main ();

  return 0;
}

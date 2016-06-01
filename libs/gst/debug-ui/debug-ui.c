/* gst-editor
 * Copyright (C) <2001> Steve Baker <stevebaker_org@yahoo.co.uk>
 * Copyright (C) <2002, 2003> Andy Wingo <wingo at pobox dotcom>
 * Copyright (C) <2004> Jan Schmidt <thaytan@mad.scientist.com>
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


#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glade/glade.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../common/gste-common.h"
#include "debug-ui.h"
enum
{
  COL_LEVEL,
  COL_NAME,
  COL_DESCRIPTION,
  COL_CATEGORY,
  NUM_COLS
};

enum
{
  COL_ADD_NAME,
  COL_ADD_DESCRIPTION,
  COL_ADD_CATEGORY,
  NUM_ADD_COLS
};

#define GSTE_TYPE_DEBUGUI              (gste_debugui_get_type())
#define GSTE_DEBUGUI(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSTE_TYPE_DEBUGUI, GsteDebugUI))
#define GSTE_DEBUGUI_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GSTE_TYPE_DEBUGUI, GsteDebugUIClass))
#define GSTE_IS_DEBUGUI(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSTE_TYPE_DEBUGUI))
#define GSTE_IS_DEBUGUI_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GSTE_TYPE_DEBUGUI))
#define GSTE_DEBUGUI_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GSTE_TYPE_DEBUGUI, GsteDebugUIClass))

typedef struct _GsteDebugUIClass GsteDebugUIClass;
typedef struct _GsteDebugUI GsteDebugUI;

struct _GsteDebugUI
{
  GtkBin parent;
  GtkTreeView *treeview;
  GtkListStore *list_store;
  GtkHScale *default_hscale;
  GtkLabel *default_label;

  GtkHScale *custom_hscale;
  GtkLabel *custom_label;

  GtkWidget *custom_box;
  GList *priv_list;

  gboolean handling_select;

  GtkWindow *add_window;
  GtkTreeView *add_cats_treeview;
};

struct _GsteDebugUIClass
{
  GtkBinClass parent_class;
};

static void gste_debugui_class_init (GsteDebugUIClass * class);
static void gste_debugui_init (GsteDebugUI * debug_ui);
static void gste_debugui_dispose (GObject * object);
static void gste_debugui_size_request (GtkWidget * widget,
    GtkRequisition * requisition);
static void gste_debugui_size_allocate (GtkWidget * widget,
    GtkAllocation * allocation);
static void tree_select (GsteDebugUI * debug_ui);
static void remove_custom_cats (GtkButton * button, GsteDebugUI * debug_ui);
static void show_add_window (GtkButton * button, GsteDebugUI * debug_ui);
static void populate_add_categories (GsteDebugUI * debug_ui,
    GtkTreeView * tview);

static GtkBinClass *parent_class = NULL;

static GType
gste_debugui_get_type (void)
{
  static GType gste_debugui_type = 0;

  if (!gste_debugui_type) {
    static const GTypeInfo gste_debugui_info = {
      sizeof (GsteDebugUIClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gste_debugui_class_init,
      NULL,
      NULL,
      sizeof (GsteDebugUI),
      0,
      (GInstanceInitFunc) gste_debugui_init,
    };

    gste_debugui_type =
        g_type_register_static (GTK_TYPE_BIN, "GsteDebugUI",
        &gste_debugui_info, 0);
  }
  return gste_debugui_type;
};

/*
 * Walk the tree model of categories and update the 'level' column
 * to reflect the current threshold
 */
static void
refresh_categories (GtkWidget * button, GtkWidget * widget)
{
  GsteDebugUI *debug_ui = GSTE_DEBUGUI (widget);
  GtkListStore *lstore;
  GtkTreeIter iter;
  gboolean valid;

  lstore = debug_ui->list_store;

  /* Can be called while still initialising the treeview */
  if (!lstore)
    return;

  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (lstore), &iter);

  while (valid) {
    GstDebugCategory *cat;
    const gchar *level;

    gtk_tree_model_get (GTK_TREE_MODEL (lstore), &iter, COL_CATEGORY, &cat, -1);
    level = gst_debug_level_get_name (gst_debug_category_get_threshold (cat));
    gtk_list_store_set (lstore, &iter,
        COL_NAME, gst_debug_category_get_name (cat),
        COL_DESCRIPTION, gst_debug_category_get_description (cat),
        COL_LEVEL, level, -1);

    valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (lstore), &iter);
  }

  populate_add_categories (debug_ui, debug_ui->add_cats_treeview);
}

static void
set_default_level (GtkAdjustment * adj, GtkWidget * widget)
{
  GsteDebugUI *debug_ui = GSTE_DEBUGUI (widget);

  gst_debug_set_default_threshold ((int) adj->value);
  gtk_label_set_label (debug_ui->default_label,
      gst_debug_level_get_name ((int) adj->value));
  refresh_categories (NULL, GTK_WIDGET (debug_ui));
}

static void
set_custom_level (GtkAdjustment * adj, GtkWidget * widget)
{
  GsteDebugUI *debug_ui = GSTE_DEBUGUI (widget);
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GList *selected_rows;
  GList *node;
  gint debug_level;

  debug_level = (gint) gtk_adjustment_get_value (adj);

  gtk_label_set_label (debug_ui->custom_label,
      gst_debug_level_get_name ((gint) debug_level));

  /* Walk through selected items in the list and set the debug category */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (debug_ui->treeview));
  selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);

  for (node = selected_rows; node != NULL; node = g_list_next (node)) {
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) node->data)) {
      GstDebugCategory *cat;

      gtk_tree_model_get (model, &iter, COL_CATEGORY, &cat, -1);
      gst_debug_category_reset_threshold (cat);
      gst_debug_category_set_threshold (cat, debug_level);
    }
  }

  g_list_foreach (selected_rows, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (selected_rows);
  refresh_categories (NULL, GTK_WIDGET (debug_ui));
}

/* Add any starting debug levels to the tree */
static void
init_custom_levels (GsteDebugUI * debug_ui)
{
  GstDebugLevel default_thresh;
  GtkListStore *lstore;
  GtkTreeIter iter;
  GSList *slist;

  lstore = debug_ui->list_store =
      gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_POINTER);

  /* 
   * Get the current default threshold. Add all categories set to a different level to the custom list
   * and refresh the tree
   */
  default_thresh = gst_debug_get_default_threshold ();
  slist = gst_debug_get_all_categories ();

  while (slist) {
    GstDebugCategory *cat = (GstDebugCategory *) slist->data;
    GstDebugLevel thresh = gst_debug_category_get_threshold (cat);

    if (thresh != default_thresh) {
      const gchar *level =
          gst_debug_level_get_name (gst_debug_category_get_threshold (cat));

      gtk_list_store_append (lstore, &iter);
      gtk_list_store_set (lstore, &iter,
          COL_NAME, gst_debug_category_get_name (cat),
          COL_DESCRIPTION, gst_debug_category_get_description (cat),
          COL_LEVEL, level, COL_CATEGORY, cat, -1);
    }
    slist = slist->next;
  }

  /* I don't really grok the sorting stuff atm, and I have to do other work
   * now */
  gtk_tree_view_set_model (GTK_TREE_VIEW (debug_ui->treeview),
      gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (lstore)));
}

GtkWidget *
gst_debug_ui_new (void)
{
  return GTK_WIDGET (g_object_new (gste_debugui_get_type (), NULL));
}

static void
gste_debugui_class_init (GsteDebugUIClass * class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_ref (GTK_TYPE_BIN);

  object_class->dispose = gste_debugui_dispose;

  widget_class->size_request = gste_debugui_size_request;
  widget_class->size_allocate = gste_debugui_size_allocate;
}

/* given the root, return the glade_xml in the glade file for that root */
static GladeXML *
gste_debugui_get_xml (const gchar * root)
{
  char *path;
  GladeXML *result;

  path = gste_get_ui_file ("editor.glade2");
  if (!path) {
    g_warning ("editor.glade2 not found in uninstalled or installed dir.");
    return NULL;
  }

  result = glade_xml_new (path, root, NULL);
  g_free (path);
  return (result);
}

static void
gste_debugui_init (GsteDebugUI * debug_ui)
{
  GladeXML *xml;
  GtkWidget *box;
  GtkWidget *add_button, *remove_button, *refresh_button;
  GtkAdjustment *adj;
  GtkTreeSelection *selection;
  GtkTreeViewColumn *column;

  debug_ui->handling_select = FALSE;
  debug_ui->list_store = NULL;
  debug_ui->priv_list = NULL;
  debug_ui->add_window = NULL;

  xml = gste_debugui_get_xml ("debug_palette");
  if (!xml) {
    g_critical ("GstEditor user interface file %s not found. "
        "Try running from the Gst-Editor source directory.", "editor.glade2");
    return;
  }

  box = glade_xml_get_widget (xml, "debug_palette");
  debug_ui->treeview =
      GTK_TREE_VIEW (glade_xml_get_widget (xml, "custom-levels-treeview"));
  debug_ui->default_hscale =
      GTK_HSCALE (glade_xml_get_widget (xml, "default-level-hscale"));
  debug_ui->default_label =
      GTK_LABEL (glade_xml_get_widget (xml, "default-level-label"));
  debug_ui->custom_hscale =
      GTK_HSCALE (glade_xml_get_widget (xml, "custom-level-hscale"));
  debug_ui->custom_label =
      GTK_LABEL (glade_xml_get_widget (xml, "custom-level-label"));
  debug_ui->custom_box = GTK_WIDGET (glade_xml_get_widget (xml, "custom-box"));
  add_button = GTK_WIDGET (glade_xml_get_widget (xml, "add-button"));
  remove_button = GTK_WIDGET (glade_xml_get_widget (xml, "remove-button"));
  refresh_button = GTK_WIDGET (glade_xml_get_widget (xml, "refresh-button"));
  g_object_unref (xml);

  g_return_if_fail (GTK_IS_TREE_VIEW (debug_ui->treeview));
  g_return_if_fail (GTK_IS_HSCALE (debug_ui->default_hscale));
  g_return_if_fail (GTK_IS_HSCALE (debug_ui->custom_hscale));
  g_return_if_fail (GTK_IS_LABEL (debug_ui->default_label));
  g_return_if_fail (GTK_IS_LABEL (debug_ui->custom_label));
  g_return_if_fail (GTK_IS_WIDGET (debug_ui->custom_box));
  g_return_if_fail (GTK_IS_WIDGET (add_button));
  g_return_if_fail (GTK_IS_WIDGET (remove_button));
  g_return_if_fail (GTK_IS_WIDGET (refresh_button));

  adj = gtk_range_get_adjustment (GTK_RANGE (debug_ui->default_hscale));
  g_signal_connect (adj,
      "value-changed", G_CALLBACK (set_default_level), debug_ui);
  gtk_adjustment_set_value (adj, gst_debug_get_default_threshold ());
  adj->value = gst_debug_get_default_threshold ();
  set_default_level (adj, GTK_WIDGET (debug_ui));

  adj = gtk_range_get_adjustment (GTK_RANGE (debug_ui->custom_hscale));
  g_signal_connect (adj,
      "value-changed", G_CALLBACK (set_custom_level), debug_ui);

  init_custom_levels (debug_ui);

  column = gtk_tree_view_column_new_with_attributes
      ("Level", gtk_cell_renderer_text_new (), "text", COL_LEVEL, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (debug_ui->treeview), column);

  column = gtk_tree_view_column_new_with_attributes
      ("Name", gtk_cell_renderer_text_new (), "text", COL_NAME, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (debug_ui->treeview), column);

  column = gtk_tree_view_column_new_with_attributes
      ("Description", gtk_cell_renderer_text_new (), "text", COL_DESCRIPTION,
      NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (debug_ui->treeview), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (debug_ui->treeview));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  g_signal_connect_swapped (selection,
      "changed", G_CALLBACK (tree_select), debug_ui);

  g_signal_connect (add_button,
      "clicked", G_CALLBACK (show_add_window), debug_ui);
  g_signal_connect (remove_button,
      "clicked", G_CALLBACK (remove_custom_cats), debug_ui);
  g_signal_connect (refresh_button,
      "clicked", G_CALLBACK (refresh_categories), debug_ui);

  gtk_container_add (GTK_CONTAINER (debug_ui), box);
  gtk_widget_show_all (GTK_WIDGET (debug_ui));
}

static void
gste_debugui_dispose (GObject * object)
{
  GsteDebugUI *debug_ui = GSTE_DEBUGUI (object);

  if (debug_ui->add_window) {
    gtk_widget_hide (GTK_WIDGET (debug_ui->add_window));
    g_object_unref (G_OBJECT (debug_ui->add_window));
    debug_ui->add_window = NULL;
  }
  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (debug_ui));
}

static void
gste_debugui_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
  GtkBin *bin = GTK_BIN (widget);
  GtkRequisition child_requisition;

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child)) {
    gtk_widget_size_request (bin->child, &child_requisition);
    requisition->width = child_requisition.width;
    requisition->height = child_requisition.height;
  } else {
    requisition->width = requisition->height = 0;
  }
}

static void
gste_debugui_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
  GtkBin *bin = GTK_BIN (widget);

  widget->allocation = *allocation;

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child)) {
    gtk_widget_size_allocate (bin->child, allocation);
  }
}

static void
tree_select (GsteDebugUI * debug_ui)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GList *selected_rows;
  GList *node;

  if (debug_ui->handling_select)
    return;

  debug_ui->handling_select = TRUE;

  /* if nothing is selected, disable the custom hscale */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (debug_ui->treeview));
  if (gtk_tree_selection_count_selected_rows (selection) == 0) {
    gtk_widget_set_sensitive (debug_ui->custom_box, FALSE);
    return;
  } else
    gtk_widget_set_sensitive (debug_ui->custom_box, TRUE);

  selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);

  for (node = selected_rows; node != NULL; node = g_list_next (node)) {
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) node->data)) {
      GtkAdjustment *adj;
      GstDebugCategory *cat;

      gtk_tree_model_get (model, &iter, COL_CATEGORY, &cat, -1);
      adj = gtk_range_get_adjustment (GTK_RANGE (debug_ui->custom_hscale));
      gtk_adjustment_set_value (adj, gst_debug_category_get_threshold (cat));
      break;
    }
  }

  g_list_foreach (selected_rows, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (selected_rows);

  debug_ui->handling_select = FALSE;
}

/* Collect a row reference for each selected row so we can delete them */
static void
collect_selected (GtkTreeModel * model, GtkTreePath * path,
    GtkTreeIter * iter, gpointer data)
{
  GsteDebugUI *debug_ui = GSTE_DEBUGUI (data);
  GtkTreeRowReference *ref;

  ref = gtk_tree_row_reference_new (model, path);
  debug_ui->priv_list = g_list_prepend (debug_ui->priv_list, (gpointer) ref);
}

static void
remove_custom_cats (GtkButton * button, GsteDebugUI * debug_ui)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GList *node;

  /* Walk through selected items in the list and set the debug category */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (debug_ui->treeview));
  gtk_tree_selection_selected_foreach (selection,
      collect_selected, (gpointer) debug_ui);

  model = gtk_tree_view_get_model (debug_ui->treeview);
  g_assert (GTK_IS_TREE_MODEL_SORT (model));

  for (node = g_list_first (debug_ui->priv_list); node != NULL;
      node = g_list_next (node)) {
    GtkTreePath *path;
    GtkTreeIter iter;
    GtkTreeIter child_iter;
    GstDebugCategory *cat;

    path =
        gtk_tree_row_reference_get_path ((GtkTreeRowReference *) (node->data));
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_path_free (path);
    gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (model),
        &child_iter, &iter);
    gtk_tree_model_get (GTK_TREE_MODEL (debug_ui->list_store), &child_iter,
        COL_CATEGORY, &cat, -1);
    gst_debug_category_reset_threshold (cat);

    gtk_list_store_remove (debug_ui->list_store, &child_iter);
    gtk_tree_row_reference_free ((GtkTreeRowReference *) (node->data));
  }
  g_list_free (debug_ui->priv_list);
  debug_ui->priv_list = NULL;

  populate_add_categories (debug_ui, debug_ui->add_cats_treeview);
}

static void
close_add_window (GtkButton * button, GtkWindow * window)
{
  gtk_widget_hide (GTK_WIDGET (window));
}

/* 
 * Handle a click on the 'add' button inside the add categories window 
 * Grab the selected categories from the list and add them to the main window
 */
static void
handle_add_cats (GtkButton * button, GsteDebugUI * debug_ui)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GList *selected_rows;
  GList *node;

  if (!debug_ui->add_window)
    return;

  gtk_widget_hide (GTK_WIDGET (debug_ui->add_window));

  /* Walk through selected items in the list and set the debug category */
  selection =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (debug_ui->add_cats_treeview));
  selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);

  for (node = selected_rows; node != NULL; node = g_list_next (node)) {
    GtkTreePath *path;
    GtkTreeIter iter;
    GstDebugCategory *cat;
    const gchar *level;

    path = (GtkTreePath *) (node->data);
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, COL_ADD_CATEGORY, &cat, -1);

    if (!cat)
      continue;

    /* Add the category to the list of custom entries */
    level = gst_debug_level_get_name (gst_debug_category_get_threshold (cat));
    gtk_list_store_append (debug_ui->list_store, &iter);
    gtk_list_store_set (debug_ui->list_store, &iter,
        COL_NAME, gst_debug_category_get_name (cat),
        COL_DESCRIPTION, gst_debug_category_get_description (cat),
        COL_LEVEL, level, COL_CATEGORY, cat, -1);
  }

  g_list_foreach (selected_rows, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (selected_rows);
}

static gboolean
find_cat_in_customlist (GsteDebugUI * debug_ui, GstDebugCategory * cat)
{
  GtkTreeIter iter;
  gboolean valid;

  valid =
      gtk_tree_model_get_iter_first (GTK_TREE_MODEL (debug_ui->list_store),
      &iter);

  while (valid) {
    GstDebugCategory *cur_cat;

    gtk_tree_model_get (GTK_TREE_MODEL (debug_ui->list_store), &iter,
        COL_CATEGORY, &cur_cat, -1);
    if (cur_cat == cat)
      return TRUE;

    valid =
        gtk_tree_model_iter_next (GTK_TREE_MODEL (debug_ui->list_store), &iter);
  }

  return FALSE;
}

static void
populate_add_categories (GsteDebugUI * debug_ui, GtkTreeView * tview)
{
  GtkListStore *lstore;
  GSList *slist;

  if (!debug_ui->add_window)
    return;

  lstore =
      gtk_list_store_new (NUM_ADD_COLS, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_POINTER);
  slist = gst_debug_get_all_categories ();
  for (; slist != NULL; slist = g_slist_next (slist)) {
    GstDebugCategory *cat = (GstDebugCategory *) slist->data;
    GtkTreeIter iter;

    /* Don't add the category if it is already in the custom cats list */
    if (find_cat_in_customlist (debug_ui, cat))
      continue;

    gtk_list_store_append (lstore, &iter);
    gtk_list_store_set (lstore, &iter,
        COL_ADD_NAME, gst_debug_category_get_name (cat),
        COL_ADD_DESCRIPTION, gst_debug_category_get_description (cat),
        COL_ADD_CATEGORY, cat, -1);
  }

  gtk_tree_view_set_model (GTK_TREE_VIEW (tview), GTK_TREE_MODEL (lstore));
  g_object_unref (G_OBJECT (lstore));
}

static void
show_add_window (GtkButton * button, GsteDebugUI * debug_ui)
{
  GladeXML *xml;

  if (!debug_ui->add_window) {
    GtkWidget *add_button, *cancel_button;
    GtkTreeSelection *selection;
    GtkTreeViewColumn *column;

    xml = gste_debugui_get_xml ("add-debug-win");
    if (!xml) {
      g_critical ("GstEditor user interface file %s not found. "
          "Try running from the Gst-Editor source directory.", "editor.glade2");
      return;
    }
    debug_ui->add_window =
        GTK_WINDOW (glade_xml_get_widget (xml, "add-debug-win"));
    g_object_ref (debug_ui->add_window);

    gtk_window_set_transient_for (GTK_WINDOW (debug_ui->add_window),
        GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (debug_ui))));

    /* Connect the callbacks and populate the list of debug categories */
    debug_ui->add_cats_treeview =
        GTK_TREE_VIEW (glade_xml_get_widget (xml, "categories-tree"));
    add_button = glade_xml_get_widget (xml, "add-button");
    cancel_button = glade_xml_get_widget (xml, "cancel-button");

    g_signal_connect (add_button,
        "clicked", G_CALLBACK (handle_add_cats), debug_ui);
    g_signal_connect (cancel_button,
        "clicked", G_CALLBACK (close_add_window), debug_ui->add_window);

    /* Set up the tree view columns */
    column = gtk_tree_view_column_new_with_attributes
        ("Name", gtk_cell_renderer_text_new (), "text", COL_ADD_NAME, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (debug_ui->add_cats_treeview),
        column);

    column = gtk_tree_view_column_new_with_attributes
        ("Description", gtk_cell_renderer_text_new (), "text",
        COL_ADD_DESCRIPTION, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (debug_ui->add_cats_treeview),
        column);

    selection =
        gtk_tree_view_get_selection (GTK_TREE_VIEW (debug_ui->
            add_cats_treeview));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  }
  populate_add_categories (debug_ui, debug_ui->add_cats_treeview);
  gtk_widget_show_all (GTK_WIDGET (debug_ui->add_window));
}

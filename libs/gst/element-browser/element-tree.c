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

#include "config.h"

#include "element-tree.h"

#include <string.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gst/common/gste-common.h>
#include <gst/common/gste-marshal.h>

struct element_select_classlist
{
  gchar *name;
  GSList *subclasses;
  GSList *factories;
};

enum
{
  NAME_COLUMN,
  DESCRIPTION_COLUMN,
  FACTORY_COLUMN,
  NUM_COLUMNS
};

enum
{
  PROP_0,
  PROP_SELECTED,
};

enum
{
  SIGNAL_SELECTED,
  SIGNAL_ACTIVATED,
  SIGNAL_LAST
};

static void gst_element_browser_element_tree_init (GstElementBrowserElementTree
    * browser);
static void
gst_element_browser_element_tree_class_init (GstElementBrowserElementTreeClass *
    browser);
static void gst_element_browser_element_tree_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_element_browser_element_tree_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_element_browser_size_request (GtkWidget * widget,
    GtkRequisition * requisition);
static void gst_element_browser_size_allocate (GtkWidget * widget,
    GtkAllocation * allocation);


static gint compare_name (gconstpointer a, gconstpointer b);
static gint compare_str (gconstpointer a, gconstpointer b);
static gint compare_classes (gconstpointer a, gconstpointer b);

static void populate_store (GtkTreeStore * store,
    GtkTreeIter * parent, struct element_select_classlist *class);
static gboolean tree_select_function (GtkTreeSelection * selection,
    GtkTreeModel * model, GtkTreePath * path, gboolean foo, gpointer data);
static void tree_select (GstElementBrowserElementTree * tree);
static void tree_activate (GstElementBrowserElementTree * tree,
    GtkTreePath * path, GtkTreeViewColumn * column);
static GSList *get_class_tree (void);
static gboolean filter_elements (GstElementBrowserElementTree * tree);
static void filter_text_changed (GstElementBrowserElementTree * tree);

static GtkBinClass *parent_class = NULL;
static guint browser_signals[SIGNAL_LAST] = { 0 };

GType
gst_element_browser_element_tree_get_type (void)
{
  static GType element_tree_type = 0;

  if (!element_tree_type) {
    static const GTypeInfo element_tree_info = {
      sizeof (GstElementBrowserElementTreeClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_element_browser_element_tree_class_init,
      NULL,
      NULL,
      sizeof (GstElementBrowserElementTree),
      0,
      (GInstanceInitFunc) gst_element_browser_element_tree_init,
    };

    element_tree_type =
        g_type_register_static (GTK_TYPE_BIN,
        "GstElementBrowserElementTree", &element_tree_info, 0);
  }
  return element_tree_type;
}

static void
gst_element_browser_element_tree_class_init (GstElementBrowserElementTreeClass *
    klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_ref (GTK_TYPE_BIN);

  gobject_class->set_property = gst_element_browser_element_tree_set_property;
  gobject_class->get_property = gst_element_browser_element_tree_get_property;

  g_object_class_install_property (gobject_class, PROP_SELECTED,
      g_param_spec_object ("selected", "Selected", "Selected element factory",
          gst_element_factory_get_type (), G_PARAM_READWRITE));

  browser_signals[SIGNAL_SELECTED] =
      g_signal_new ("element-selected",
      G_OBJECT_CLASS_TYPE (gobject_class),
      G_SIGNAL_RUN_FIRST,
      G_STRUCT_OFFSET (GstElementBrowserElementTreeClass, selected_sig),
      NULL, NULL, gst_editor_marshal_VOID__VOID, G_TYPE_NONE, 0);

  browser_signals[SIGNAL_ACTIVATED] =
      g_signal_new ("element-activated",
      G_OBJECT_CLASS_TYPE (gobject_class),
      G_SIGNAL_RUN_FIRST,
      G_STRUCT_OFFSET (GstElementBrowserElementTreeClass, activated_sig),
      NULL, NULL, gst_editor_marshal_VOID__VOID, G_TYPE_NONE, 0);

  widget_class->size_request = gst_element_browser_size_request;
  widget_class->size_allocate = gst_element_browser_size_allocate;
  klass->selected_sig = NULL;
  klass->activated_sig = NULL;
}

static void
gst_element_browser_element_tree_init (GstElementBrowserElementTree * tree)
{
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GSList *classtree;
  GladeXML *xml;
  GtkWidget *palette, *tview, *find_box;
  gchar *path;

  path = gste_get_ui_file ("editor.glade2");
  if (!path)
    g_error ("GStreamer Editor user interface file 'editor.glade2' not found.");
  xml = glade_xml_new (path, "quicklaunch_palette", NULL);

  if (!xml) {
    g_error ("GStreamer Editor could not load quicklaunch_palette from %s",
        path);
  }
  g_free (path);

  palette = glade_xml_get_widget (xml, "quicklaunch_palette");
  tview = glade_xml_get_widget (xml, "element-tree");
  find_box = glade_xml_get_widget (xml, "find-entry");

  g_object_unref (xml);
  g_return_if_fail (GTK_IS_TREE_VIEW (tview));
  g_return_if_fail (GTK_IS_ENTRY (find_box));

  tree->filter_entry = GTK_ENTRY (find_box);
  tree->view = tview;
  tree->store = gtk_tree_store_new (NUM_COLUMNS,
      G_TYPE_STRING, G_TYPE_STRING, GST_TYPE_ELEMENT_FACTORY);
  tree->cur_model = GTK_TREE_MODEL (tree->store);
  gtk_tree_view_set_model (GTK_TREE_VIEW (tview), tree->cur_model);

  column = gtk_tree_view_column_new_with_attributes ("Element",
      gtk_cell_renderer_text_new (), "text", NAME_COLUMN, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree->view), column);
  column = gtk_tree_view_column_new_with_attributes ("Description",
      gtk_cell_renderer_text_new (), "text", DESCRIPTION_COLUMN, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree->view), column);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree->view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  gtk_tree_selection_set_select_function (selection, tree_select_function, NULL,
      NULL);

  tree->filter_store = gtk_list_store_new (NUM_COLUMNS,
      G_TYPE_STRING, G_TYPE_STRING, GST_TYPE_ELEMENT_FACTORY);

  gtk_widget_show_all (GTK_WIDGET (tree));
  gtk_container_add (GTK_CONTAINER (tree), palette);

  for (classtree = get_class_tree (); classtree; classtree = classtree->next)
    populate_store (tree->store, NULL,
        (struct element_select_classlist *) (classtree->data));

  g_signal_connect_swapped (tree->view, "row-activated",
      G_CALLBACK (tree_activate), tree);
  g_signal_connect_swapped (selection, "changed",
      G_CALLBACK (tree_select), tree);
  g_signal_connect_swapped (find_box, "changed",
      G_CALLBACK (filter_text_changed), tree);

  tree->current_filter_text = g_strdup ("");
  tree->filter_idle_id = 0;
}

static void
gst_element_browser_element_tree_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstElementBrowserElementTree *tree;

  tree = GST_ELEMENT_BROWSER_ELEMENT_TREE (object);

  switch (prop_id) {
    case PROP_SELECTED:
      tree->selected = (GstElementFactory *) g_value_get_object (value);
      g_warning ("I am mostly nonfunctional..");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }
}

static void
gst_element_browser_element_tree_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstElementBrowserElementTree *tree;

  tree = GST_ELEMENT_BROWSER_ELEMENT_TREE (object);

  switch (prop_id) {
    case PROP_SELECTED:
      g_value_set_object (value, (GObject *) tree->selected);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_element_browser_size_request (GtkWidget * widget,
    GtkRequisition * requisition)
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
gst_element_browser_size_allocate (GtkWidget * widget,
    GtkAllocation * allocation)
{
  GtkBin *bin = GTK_BIN (widget);

  widget->allocation = *allocation;

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child)) {
    gtk_widget_size_allocate (bin->child, allocation);
  }
}

GtkWidget *
gst_element_browser_element_tree_new ()
{
  return GTK_WIDGET (g_object_new (gst_element_browser_element_tree_get_type (),
          NULL));
}

static void
populate_store (GtkTreeStore * store, GtkTreeIter * parent,
    struct element_select_classlist *class)
{
  GSList *traverse;
  GtkTreeIter iter, newparent;

  gtk_tree_store_append (store, &iter, parent);
  gtk_tree_store_set (store, &iter,
      NAME_COLUMN, class->name, DESCRIPTION_COLUMN, NULL, -1);
  newparent = iter;

  /* start with a sort and save to original GSList location */
  traverse = g_slist_sort (class->subclasses, compare_classes);
  class->subclasses = traverse;
  while (traverse) {
    populate_store (store, &newparent,
        (struct element_select_classlist *) (traverse->data));
    traverse = g_slist_next (traverse);
  }

  traverse = class->factories;
  while (traverse) {
    GstElementFactory *factory = (GstElementFactory *) (traverse->data);

    gtk_tree_store_append (store, &iter, &newparent);
    gtk_tree_store_set (store, &iter,
        NAME_COLUMN, gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (factory)),
        DESCRIPTION_COLUMN, gst_element_factory_get_description (factory),
        FACTORY_COLUMN, factory, -1);
    traverse = g_slist_next (traverse);
  }
}

static gboolean
tree_select_function (GtkTreeSelection * selection, GtkTreeModel * model,
    GtkTreePath * path, gboolean foo, gpointer data)
{
  GtkTreeIter iter;
  GstElementFactory *factory = NULL;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, FACTORY_COLUMN, &factory, -1);

  if (factory)
    return TRUE;
  else
    return FALSE;
}

static void
tree_select (GstElementBrowserElementTree * tree)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GstElementFactory *factory;

  /* if nothing is selected, ignore it */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree->view));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return;

  gtk_tree_model_get (model, &iter, FACTORY_COLUMN, &factory, -1);

  g_return_if_fail (factory != NULL);

  tree->selected = factory;
  g_signal_emit (tree, browser_signals[SIGNAL_SELECTED], 0);
}

static void
tree_activate (GstElementBrowserElementTree * tree, GtkTreePath * path,
    GtkTreeViewColumn * column)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GstElementFactory *factory;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree->view));
  if (!gtk_tree_model_get_iter (model, &iter, path))
    return;

  gtk_tree_model_get (model, &iter, FACTORY_COLUMN, &factory, -1);

  if (!factory)
    return;

  tree->selected = factory;
  g_signal_emit (tree, browser_signals[SIGNAL_ACTIVATED], 0);
}

static gboolean
set_select_by_factory_subtree (GstElementBrowserElementTree * tree,
    GtkTreeIter * iter, GstElementFactory * sel_factory)
{
  gboolean valid = TRUE;
  GtkTreeIter child_iter;
  GstElementFactory *factory;

  /* Walk forward searching for the element factory */
  while (valid) {
    gtk_tree_model_get (tree->cur_model, iter, FACTORY_COLUMN, &factory, -1);
    if (factory == sel_factory) {
      GtkTreeSelection *selection =
          gtk_tree_view_get_selection (GTK_TREE_VIEW (tree->view));
      GtkTreePath *path;

      path = gtk_tree_model_get_path (tree->cur_model, iter);
      if (path) {
        gtk_tree_view_collapse_all (GTK_TREE_VIEW (tree->view));
        gtk_tree_view_expand_to_path (GTK_TREE_VIEW (tree->view), path);
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (tree->view), path, NULL,
            FALSE, 0, 0);
        gtk_tree_path_free (path);
      }

      gtk_tree_selection_select_iter (selection, iter);
      return TRUE;
    }

    if (gtk_tree_model_iter_children (tree->cur_model, &child_iter, iter))
      if (set_select_by_factory_subtree (tree, &child_iter, sel_factory))
        return TRUE;

    valid = gtk_tree_model_iter_next (tree->cur_model, iter);
  }

  return FALSE;
}

static void
set_tree_selection_by_factory (GstElementBrowserElementTree * tree,
    GstElementFactory * sel_factory)
{
  GtkTreeIter iter;
  gboolean valid = FALSE;

  valid = gtk_tree_model_get_iter_first (tree->cur_model, &iter);
  if (valid)
    set_select_by_factory_subtree (tree, &iter, sel_factory);
}

static void
set_tree_model (GstElementBrowserElementTree * tree, GtkTreeModel * model)
{
  if (tree->cur_model != model) {
    GstElementFactory *factory = tree->selected;

    tree->cur_model = model;
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree->view), tree->cur_model);

    /* If there's a selection in the filtered list, expand that item in the tree
view */
    if (factory)
      set_tree_selection_by_factory (tree, factory);
  }
}

/* Walk through the tree model and filter entries into the filter list_store */
static void
filter_subtree (GstElementBrowserElementTree * tree, GtkTreeIter * iter)
{
  gboolean valid = TRUE;
  GtkTreeIter child_iter;
  GstElementFactory *factory = NULL;

  /* Walk forward matching element factories that contain the search text */
  while (valid) {
    gtk_tree_model_get (GTK_TREE_MODEL (tree->store), iter, FACTORY_COLUMN,
        &factory, -1);

    if ((factory != NULL)
        && (strstr (gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (factory)),
                tree->current_filter_text) != NULL)) {

      GtkTreeIter new_iter;

      /* Add the matching row to the filtered list */
      gtk_list_store_append (tree->filter_store, &new_iter);
      gtk_list_store_set (tree->filter_store, &new_iter,
          NAME_COLUMN, gst_plugin_feature_get_name (GST_PLUGIN_FEATURE
(factory)),
          DESCRIPTION_COLUMN, factory->details.description,
          FACTORY_COLUMN, factory, -1);
    }

    if (gtk_tree_model_iter_children (GTK_TREE_MODEL (tree->store), &child_iter,
            iter))
      filter_subtree (tree, &child_iter);

    valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (tree->store), iter);
  }
}

/* 
 * Clear the filter_model, walk the entire element tree adding
 * matching entries
 */
static void
build_filter_by_tree (GstElementBrowserElementTree * tree)
{
  GtkTreeIter iter;
  gboolean valid = FALSE;
  GstElementFactory *factory = tree->selected;

  gtk_list_store_clear (tree->filter_store);

  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (tree->store), &iter);
  if (valid) {
    filter_subtree (tree, &iter);
  }

  if (factory)
    set_tree_selection_by_factory (tree, factory);
}

/*
 * Walk the existing filter list model, removing non-matching entries
 */
static void
build_filter_by_existing (GstElementBrowserElementTree * tree)
{
  GtkTreeIter iter;
  GtkTreeModel *model = GTK_TREE_MODEL (tree->filter_store);
  gboolean valid = gtk_tree_model_get_iter_first (model, &iter);
  GstElementFactory *factory = NULL;

  while (valid) {
    gtk_tree_model_get (model, &iter, FACTORY_COLUMN, &factory, -1);
    if ((factory == NULL)
        || (strstr (gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (factory)),
                tree->current_filter_text) == NULL))
      valid = gtk_list_store_remove (tree->filter_store, &iter);
    else
      valid = gtk_tree_model_iter_next (model, &iter);
  }
}

/*
 * Retrieve the text from the filter text and:
 * If the text is the previous text + a suffix, whittle down the filter_model
entries
 * If the text is not, clear the filter_model and rebuild from the tree model
 * If the text is 0 length, redisplay the tree model
 */
static gboolean
filter_elements (GstElementBrowserElementTree * tree)
{
  const gchar *filter_text;
  gchar *old_filter_text;
  int old_ft_len;

  /* Get the text from the edit box */
  filter_text = gtk_entry_get_text (tree->filter_entry);
  if (strcmp (filter_text, tree->current_filter_text) == 0) {
    tree->filter_idle_id = 0;
    return FALSE;
  }

  /* If the new text is blank, reinstate the treemodel and clear the
filter_model */
  if (strlen (filter_text) == 0) {
    set_tree_model (tree, GTK_TREE_MODEL (tree->store));

    tree->filter_idle_id = 0;
    return FALSE;
  }

  old_filter_text = tree->current_filter_text;
  tree->current_filter_text = g_strdup (filter_text);

  /* Determine if the new text is just the old text with extra tacked on */
  /* if so, we filter down the existing filter_model to save time, otherwise
build a new filter_model list */
  old_ft_len = strlen (old_filter_text);
  if ((old_ft_len == 0) ||
      (strncmp (old_filter_text, filter_text, old_ft_len) != 0))
    build_filter_by_tree (tree);
  else
    build_filter_by_existing (tree);

  set_tree_model (tree, GTK_TREE_MODEL (tree->filter_store));

  g_free (old_filter_text);
  tree->filter_idle_id = 0;
  return FALSE;
}

static void
filter_text_changed (GstElementBrowserElementTree * tree)
{
  if (tree->filter_idle_id)
    g_source_remove (tree->filter_idle_id);

  tree->filter_idle_id = g_idle_add ((GSourceFunc) (filter_elements), tree);
}

static GSList *
get_class_tree (void)
{
  GList *elements, *node;
  GstElementFactory *element;
  gchar **classes, **class;
  GSList *classtree, *treewalk;
  GSList **curlist;
  struct element_select_classlist *branch = NULL;
  static GSList *ret = NULL;

  if (ret)
    return ret;

  /* first create a sorted (by class) tree of all the factories */
  classtree = NULL;
  
elements = node =
      gst_registry_get_feature_list (gst_registry_get_default (),
      GST_TYPE_ELEMENT_FACTORY);
  while (node) {
    element = (GstElementFactory *) (node->data);

    /* if the class is "None", we just ignore it */
    if (strncmp ("None", element->details.klass, 4) == 0)
      goto loop;

    /* split up the factory's class */
    classes = g_strsplit (element->details.klass, "/", 0);
    class = classes;
    curlist = &classtree;
    /* walk down the class tree to find where to place this element
     * the goal is for treewalk to point to the right class branch
     * when we exit this thing, branch is pointing where we want
     */
    while (*class) {
      treewalk = *curlist;
      /* walk the current level of class to see if we have the class */
      while (treewalk) {
        branch = (struct element_select_classlist *) (treewalk->data);
        /* see if this class matches what we're looking for */
        if (!strcmp (branch->name, *class)) {
          /* if so, we progress down the list into this one's list */
          curlist = &branch->subclasses;
          break;
        }
        treewalk = g_slist_next (treewalk);
      }
      /* if treewalk == NULL, it wasn't in the list. add one */
      if (treewalk == NULL) {
        /* curlist is pointer to list */
        branch = g_new0 (struct element_select_classlist, 1);

        branch->name = g_strdup (*class);
        *curlist = g_slist_insert_sorted (*curlist, branch, compare_str);
        curlist = &branch->subclasses;
      }
      class++;
    }
    /* theoretically branch points where we want now */
    branch->factories = g_slist_insert_sorted (branch->factories, element,
        compare_name);
  loop:
    node = g_list_next (node);
  }
  g_list_free (elements);

  ret = g_slist_sort (classtree, compare_classes);
  return ret;
}

static gint
compare_name (gconstpointer a, gconstpointer b)
{
  return (strcmp (GST_OBJECT_NAME (a), GST_OBJECT_NAME (b)));
}

static gint
compare_str (gconstpointer a, gconstpointer b)
{
  return (strcmp ((gchar *) a, (gchar *) b));
}

static gint
compare_classes (gconstpointer a, gconstpointer b)
{
  struct element_select_classlist *A = (struct element_select_classlist *) a;
  struct element_select_classlist *B = (struct element_select_classlist *) b;

  return (strcmp ((gchar *) A->name, (gchar *) B->name));
}

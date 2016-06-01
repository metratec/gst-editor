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


#include "caps-tree.h"


enum
{
  NAME_COLUMN,
  INFO_COLUMN,
  NUM_COLUMNS
};

enum
{
  PROP_0,
  PROP_ELEMENT_FACTORY,
  PROP_ELEMENT
};

typedef struct
{
  GtkTreeStore *store;
  GtkTreeIter *parent;
} CapsTreeTarget;

static void gst_element_browser_caps_tree_init (GstElementBrowserCapsTree * ct);
static void
gst_element_browser_caps_tree_class_init (GstElementBrowserCapsTreeClass *
    klass);
static void gst_element_browser_caps_tree_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_element_browser_caps_tree_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

static void add_caps_to_tree (GstCaps * caps, GtkTreeStore * store,
    GtkTreeIter * parent);
static gboolean add_field_to_tree (GQuark field_id, const GValue * value,
    gpointer target);

static gchar *print_value (const GValue * value);
static void update_caps_tree (GstElementBrowserCapsTree * ct);


static GtkScrolledWindowClass *parent_class = NULL;


GType
gst_element_browser_caps_tree_get_type (void)
{
  static GType element_browser_caps_tree_type = 0;

  if (!element_browser_caps_tree_type) {
    static const GTypeInfo element_browser_caps_tree_info = {
      sizeof (GstElementBrowserCapsTreeClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_element_browser_caps_tree_class_init,
      NULL,
      NULL,
      sizeof (GstElementBrowserCapsTree),
      0,
      (GInstanceInitFunc) gst_element_browser_caps_tree_init,
    };
    element_browser_caps_tree_type =
        g_type_register_static (gtk_scrolled_window_get_type (),
        "GstElementBrowserCapsTree", &element_browser_caps_tree_info, 0);
  }
  return element_browser_caps_tree_type;
}

static void
gst_element_browser_caps_tree_class_init (GstElementBrowserCapsTreeClass *
    klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  parent_class = (GtkScrolledWindowClass *)
      g_type_class_ref (gtk_scrolled_window_get_type ());

  gobject_class->set_property = gst_element_browser_caps_tree_set_property;
  gobject_class->get_property = gst_element_browser_caps_tree_get_property;

  g_object_class_install_property (gobject_class, PROP_ELEMENT_FACTORY,
      g_param_spec_object ("element-factory", "Element factory",
          "Element factory", gst_element_factory_get_type (),
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ELEMENT,
      g_param_spec_object ("element", "Element", "Element",
          gst_element_get_type (), G_PARAM_READWRITE));
}

static void
gst_element_browser_caps_tree_init (GstElementBrowserCapsTree * ct)
{
  GtkTreeViewColumn *column;

  g_object_set (G_OBJECT (ct), "hadjustment", NULL, "vadjustment", NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ct), GTK_POLICY_NEVER,
      GTK_POLICY_AUTOMATIC);

  ct->store = gtk_tree_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
  ct->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ct->store));
  gtk_tree_view_set_model (GTK_TREE_VIEW (ct->treeview),
      GTK_TREE_MODEL (ct->store));
  column =
      gtk_tree_view_column_new_with_attributes ("Name",
      gtk_cell_renderer_text_new (), "text", NAME_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ct->treeview), column);
  column = gtk_tree_view_column_new_with_attributes ("Info",
      gtk_cell_renderer_text_new (), "text", INFO_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ct->treeview), column);

  gtk_widget_show (ct->treeview);
  gtk_container_add (GTK_CONTAINER (ct), ct->treeview);
  gtk_widget_show (GTK_WIDGET (ct));
}

static void
gst_element_browser_caps_tree_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstElementBrowserCapsTree *ct;

  ct = GST_ELEMENT_BROWSER_CAPS_TREE (object);

  switch (prop_id) {
    case PROP_ELEMENT_FACTORY:
      ct->factory = (GstElementFactory *) g_value_get_object (value);
      break;
    case PROP_ELEMENT:
      ct->element = (GstElement *) g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }

  update_caps_tree (ct);
}

static void
gst_element_browser_caps_tree_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstElementBrowserCapsTree *ct;

  ct = GST_ELEMENT_BROWSER_CAPS_TREE (object);

  switch (prop_id) {
    case PROP_ELEMENT_FACTORY:
      g_value_set_object (value, (GObject *) ct->factory);
      break;
    case PROP_ELEMENT:
      g_value_set_object (value, (GObject *) ct->element);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

GtkWidget *
gst_element_browser_caps_tree_new ()
{
  return GTK_WIDGET (g_object_new (gst_element_browser_caps_tree_get_type (),
          NULL));
}

static void
update_caps_tree (GstElementBrowserCapsTree * ct)
{
  const GList *templs;
  GtkTreeStore *store;
  GtkTreeIter iter;
  GtkTreePath *path;

  store = ct->store;

  gtk_tree_store_clear (store);

  if (ct->element) {
    GstIterator *it;
    gboolean done = FALSE;

    it = gst_element_iterate_pads (ct->element);
    while (!done) {
      gpointer item;

      switch (gst_iterator_next (it, &item)) {
        case GST_ITERATOR_OK:
        {
          GstPad *pad = GST_PAD (item);
          GstCaps *caps = gst_pad_get_caps (pad);

          gtk_tree_store_append (store, &iter, NULL);

          /* Add a sub-tree of caps entries */
          add_caps_to_tree (caps, store, &iter);

          gtk_tree_store_set (store, &iter,
              NAME_COLUMN, g_strdup (gst_pad_get_name (pad)),
              INFO_COLUMN,
              g_strdup (GST_PAD_IS_SINK (&pad) ? "Sink" :
                  GST_PAD_IS_SRC (&pad) ? "Source" : "Unknown pad direction"),
              -1);
          path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
          gtk_tree_view_expand_row (GTK_TREE_VIEW (ct->treeview), path, FALSE);
          gtk_tree_path_free (path);
          gst_object_unref (pad);
          break;
        }
        case GST_ITERATOR_RESYNC:
          //...rollback changes to items...
          gst_iterator_resync (it);
          break;
        case GST_ITERATOR_ERROR:
          //...wrong parameter were given...
        case GST_ITERATOR_DONE:
        default:
          done = TRUE;
          break;

      }
    }
    gst_iterator_free (it);
  }

  if (ct->factory) {
    //templs = ct->factory->staticpadtemplates;
    templs = gst_element_factory_get_static_pad_templates (ct->factory);
    while (templs) {
//      GstPadTemplate *templ = (GstPadTemplate *) templs->data;
//      GstCaps *caps = GST_PAD_TEMPLATE_CAPS (templ);
      GstPadTemplate *templ = gst_static_pad_template_get (templs->data);
      GstCaps *caps = gst_pad_template_get_caps (templ);

      gtk_tree_store_append (store, &iter, NULL);

      /* Add a sub-tree of caps entries */
      add_caps_to_tree (caps, store, &iter);

      gtk_tree_store_set (store, &iter,
          NAME_COLUMN, g_strdup (templ->name_template),
          INFO_COLUMN,
          g_strdup (GST_PAD_TEMPLATE_DIRECTION (templ) ==
              GST_PAD_SINK ? "Sink" : GST_PAD_TEMPLATE_DIRECTION (templ) ==
              GST_PAD_SRC ? "Source" : "Unknown template direction"), -1);

      path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
      gtk_tree_view_expand_row (GTK_TREE_VIEW (ct->treeview), path, FALSE);
      gtk_tree_path_free (path);

      templs = g_list_next (templs);
    }
  }
}

static void
add_caps_to_tree (GstCaps * caps, GtkTreeStore * store, GtkTreeIter * parent)
{
  GtkTreeIter iter;
  GstStructure *structure;
  const gchar *mime;
  CapsTreeTarget target;
  guint i;

  target.store = store;

  for (i = 0; i < gst_caps_get_size (caps); i++) {
    structure = gst_caps_get_structure (caps, i);
    mime = gst_structure_get_name (structure);

    gtk_tree_store_append (store, &iter, parent);
    gtk_tree_store_set (store, &iter, NAME_COLUMN, mime, -1);

    target.parent = &iter;
    gst_structure_foreach (structure, add_field_to_tree, &target);
  }
}

static gboolean
add_field_to_tree (GQuark field_id, const GValue * value, gpointer userdata)
{
  CapsTreeTarget *target = (CapsTreeTarget *) userdata;
  GtkTreeIter iter;

  gtk_tree_store_append (target->store, &iter, target->parent);
  gtk_tree_store_set (target->store, &iter,
      NAME_COLUMN, g_quark_to_string (field_id),
      INFO_COLUMN, print_value (value), -1);
  return TRUE;
}

/* this should really be a GtkTreeCellDataFunc */
static gchar *
print_value (const GValue * value)
{
  GType type = G_VALUE_TYPE (value);

  switch (type) {
    case G_TYPE_INT:
    {
      gint val;

      val = g_value_get_int (value);
      return g_strdup_printf ("Integer: %d", val);
      break;
    }
    case G_TYPE_DOUBLE:
    {
      gdouble val;

      val = g_value_get_double (value);
      return g_strdup_printf ("Double: %g", val);
      break;
    }
    case G_TYPE_BOOLEAN:
    {
      gboolean val;

      val = g_value_get_boolean (value);
      return g_strdup_printf ("Boolean: %s", val ? "TRUE" : "FALSE");
      break;
    }
    case G_TYPE_STRING:
    {
      const gchar *val;

      val = g_value_get_string (value);
      return g_strdup_printf ("String: %s", val);
      break;
    }
    default:
      if (type == GST_TYPE_INT_RANGE) {
        gint min, max;

        min = gst_value_get_int_range_min (value);
        max = gst_value_get_int_range_max (value);
        return g_strdup_printf ("Integer range: %d - %d", min, max);
      } else if (type == GST_TYPE_DOUBLE_RANGE) {
        gdouble min, max;

        min = gst_value_get_double_range_min (value);
        max = gst_value_get_double_range_max (value);
        return g_strdup_printf ("Double range: %g - %g", min, max);
      } else if (type == GST_TYPE_FOURCC) {
        guint32 val;

        val = gst_value_get_fourcc (value);
        return g_strdup_printf ("FourCC: '%c%c%c%c'",
            (gchar) (val & 0xff),
            (gchar) ((val >> 8) & 0xff),
            (gchar) ((val >> 16) & 0xff), (gchar) ((val >> 24) & 0xff));
      } else if (type == GST_TYPE_LIST) {
        GArray *array;
        GValue *list_value;
        gchar *ret;
        guint count = 0;

        array = (GArray *) g_value_peek_pointer (value);
        ret = g_strdup_printf ("List: ");
        for (count = 0; count < array->len; count++) {
          gchar *s;

          list_value = &g_array_index (array, GValue, count);
          s = print_value (list_value);
          ret = g_strconcat (ret, count ? ", " : "", s, NULL);
          g_free (s);
        }
        return ret;
      }
      return g_strdup_printf ("unknown caps field type %s", g_type_name (type));
  }
}

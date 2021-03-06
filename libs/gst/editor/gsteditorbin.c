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

#include <math.h>
#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gst/gst.h>

#include <gst/common/gste-debug.h>
#include <gst/element-browser/browser.h>

#include "gst-helper.h"
#include "gsteditorlink.h"
#include "gsteditorcanvas.h"
#include "gsteditorelement.h"
#include "gsteditoritem.h"
#include "gsteditorbin.h"

GST_DEBUG_CATEGORY (gste_bin_debug);
#define GST_CAT_DEFAULT gste_bin_debug

/* interface methods */
#if 0
static void canvas_item_interface_init (GooCanvasItemIface * iface);
static gboolean gst_editor_bin_button_press_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventButton * event);
#endif
/* class methods */
static void gst_editor_bin_base_init (GstEditorBinClass * klass);
static void gst_editor_bin_class_init (GstEditorBinClass * klass);
static void gst_editor_bin_init (GstEditorBin * bin);
static void gst_editor_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_editor_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void gst_editor_bin_resize (GstEditorItem * bin);
static void gst_editor_bin_repack (GstEditorItem * bin);
static void gst_editor_bin_object_changed (GstEditorItem * bin,
    GstObject * object);

static void gst_editor_bin_element_added (GstObject * bin, GstObject * child,
    GstEditorBin * editorbin);

/* callback on the gstbin */
static void gst_editor_bin_element_added_cb (GstBin * bin, GstElement * child,
    gpointer user_data);
static void gst_editor_bin_element_removed_cb (GstBin * bin, GstElement * child,
    gpointer user_data);

static gboolean gst_editor_bin_child_as_bin (GstEditorBin * editorbin, GstObject * child);

/* popup callbacks */
static void on_add_element (GSimpleAction * action,
    GVariant * parameter, gpointer user_data);
static void on_paste (GSimpleAction * action,
    GVariant * parameter, gpointer user_data);

enum
{
  ARG_0,
  ARG_ATTRIBUTES
};

static GstEditorElementClass *parent_class = NULL;

static const GActionEntry action_entries[] = {
      {"paste", on_paste, NULL, NULL, NULL},
      {"add", on_add_element, NULL, NULL, NULL}
};

static const char *ui_description =
    "<interface>"
      "<menu id='itemMenu'>"
        "<section id='binSection'>"
          "<item>"
            "<attribute name='label' translatable='yes'>Add element...</attribute>"
            "<attribute name='tooltip' translatable='yes'>Add an element to this bin</attribute>"
            "<attribute name='icon'>list-add</attribute>"
            "<attribute name='action'>local.add</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>_Paste</attribute>"
            "<attribute name='icon'>edit-paste</attribute>"
            "<attribute name='action'>local.paste</attribute>"
          "</item>"
        "</section>"
      "</menu>"
    "</interface>";

GType
gst_editor_bin_get_type (void)
{
  static GType bin_type = 0;

  if (!bin_type) {
    static const GTypeInfo bin_info = {
      sizeof (GstEditorBinClass),
      (GBaseInitFunc) gst_editor_bin_base_init,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gst_editor_bin_class_init,
      NULL,
      NULL,
      sizeof (GstEditorBin),
      0,
      (GInstanceInitFunc) gst_editor_bin_init,
    };
#if 0
    static const GInterfaceInfo iitem_info = {
      (GInterfaceInitFunc) canvas_item_interface_init,  /* interface_init */
      NULL,                     /* interface_finalize */
      NULL                      /* interface_data */
    };
#endif
    bin_type =
        g_type_register_static (GST_TYPE_EDITOR_ELEMENT, "GstEditorBin",
        &bin_info, 0);
#if 0
    g_type_add_interface_static (bin_type, GOO_TYPE_CANVAS_ITEM, &iitem_info);
#endif
  }
  return bin_type;
}

static void
gst_editor_bin_base_init (GstEditorBinClass * klass)
{
  GstEditorItemClass *item_class = GST_EDITOR_ITEM_CLASS (klass);
  GtkBuilder *ui_builder = gtk_builder_new_from_string (ui_description, -1);

  g_menu_prepend_section (item_class->gmenu, NULL,
      G_MENU_MODEL (gtk_builder_get_object (ui_builder, "binSection")));

  g_object_unref (ui_builder);
}

static void
gst_editor_bin_class_init (GstEditorBinClass * klass)
{
  GObjectClass *object_class;
  GstEditorItemClass *item_class;

  object_class = G_OBJECT_CLASS (klass);
  item_class = GST_EDITOR_ITEM_CLASS (klass);

  parent_class = g_type_class_ref (gst_editor_element_get_type ());

  object_class->set_property = gst_editor_bin_set_property;
  object_class->get_property = gst_editor_bin_get_property;

  g_object_class_install_property (object_class, ARG_ATTRIBUTES,
      g_param_spec_pointer ("attributes", "attributes", "attributes",
          G_PARAM_WRITABLE));

#if 0
  citem_class->event = gst_editor_bin_event;
  citem_class->realize = gst_editor_bin_realize;
#endif
  item_class->resize = gst_editor_bin_resize;
  item_class->repack = gst_editor_bin_repack;
  item_class->object_changed = gst_editor_bin_object_changed;

  GST_DEBUG_CATEGORY_INIT (gste_bin_debug, "GSTE_BIN", 0,
      "GStreamer Editor Bin");
}

static void
gst_editor_bin_init (GstEditorBin * bin)
{
  GstEditorItem *item = GST_EDITOR_ITEM (bin);

  g_action_map_add_action_entries (G_ACTION_MAP (item->action_group),
      action_entries, G_N_ELEMENTS (action_entries), bin);

  /* set these directly here, things will get repacked when the item is
   * realized */
  item->width = 300;
  item->height = 125;

  bin->element_x = -1;
  bin->element_y = -1;

  g_object_set (bin, "resizeable", TRUE, NULL);
}

#if 0
static void
canvas_item_interface_init (GooCanvasItemIface * iface)
{
  iface->button_press_event = gst_editor_bin_button_press_event;
}
#endif

static void
gst_editor_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstEditorBin *bin = GST_EDITOR_BIN (object);

  switch (prop_id) {
    case ARG_ATTRIBUTES:
      bin->attributes = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_editor_bin_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

}

/*static*/ void
gst_editor_bin_realize (GooCanvasItem * citem)
{
  GstEditorItem *item;
  GstEditorBin *bin;
  const GList *children;

  item = GST_EDITOR_ITEM (citem);
  bin = GST_EDITOR_BIN (citem);

  children = bin->elements;

  g_return_if_fail (item->object != NULL);

  EDITOR_DEBUG ("editor_bin: realize start");
  EDITOR_DEBUG ("editor_bin: attributes: %p", bin->attributes);

  gst_editor_element_realize (citem);
#if 0
  if (GNOME_CANVAS_ITEM_CLASS (parent_class)->realize)
    GNOME_CANVAS_ITEM_CLASS (parent_class)->realize (citem);
#endif

  //children = gst_bin_get_list (GST_BIN (item->object));
  g_signal_connect (item->object, "element-added",
      G_CALLBACK (gst_editor_bin_element_added_cb), bin);
  g_signal_connect (item->object, "element-removed",
      G_CALLBACK (gst_editor_bin_element_removed_cb), bin);

  children = g_list_last(GST_BIN_CHILDREN (GST_BIN (item->object)));

  while (children) {
    gst_editor_bin_element_added (item->object, GST_OBJECT (children->data),
        bin);

    children = g_list_previous (children);
  }



  if (G_OBJECT_TYPE (item) == GST_TYPE_EDITOR_BIN)
    gst_editor_item_resize (item);
}

static void
gst_editor_bin_resize (GstEditorItem * item)
{
  /* fixme: see if the elements fit or not */

  GST_EDITOR_ITEM_CLASS (parent_class)->resize (item);
  if (G_OBJECT_TYPE (item) == GST_TYPE_EDITOR_BIN) gst_editor_item_move(item,0.0,0.0);
  
  //gst_editor_element_move (GST_EDITOR_ELEMENT (item), 0, 0);
}

static void
gst_editor_bin_repack (GstEditorItem * item)
{
  if (!item->realized)
    return;

  if (GST_EDITOR_ITEM_CLASS (parent_class)->repack)
    (GST_EDITOR_ITEM_CLASS (parent_class)->repack) (item);
}

#if 0
static gint
gst_editor_bin_event (GnomeCanvasItem * citem, GdkEvent * event)
{
  GstEditorBin *bin;
  GstEditorElement *element;
  GstEditorItem *item;

  element = GST_EDITOR_ELEMENT (citem);
  bin = GST_EDITOR_BIN (citem);
  item = GST_EDITOR_ITEM (citem);

  /* we don't actually do anything, yo */
  switch (event->type) {
    case GDK_BUTTON_PRESS:
      break;
    default:
      break;
  }

  if (GNOME_CANVAS_ITEM_CLASS (parent_class)->event)
    return GNOME_CANVAS_ITEM_CLASS (parent_class)->event (citem, event);
  return TRUE;
}
#endif

#if 0
static gboolean
gst_editor_bin_button_press_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventButton * event)
{
  GstEditorBin *bin;
  GstEditorElement *element;
  GstEditorItem *item;
  GooCanvasItemIface *iface;

  element = GST_EDITOR_ELEMENT (citem);
  bin = GST_EDITOR_BIN (citem);
  item = GST_EDITOR_ITEM (citem);

  iface =
      (GooCanvasItemIface *) g_type_interface_peek (parent_class,
      GOO_TYPE_CANVAS_ITEM);
  if (iface->button_press_event)
    return iface->button_press_event (citem, target, event);

  return TRUE;
}
#endif

static void
gst_editor_bin_object_changed (GstEditorItem * item, GstObject * object)
{
  GstEditorBin *bin = GST_EDITOR_BIN (item);
  GList *l = NULL;
  GstBin *gstbin = GST_BIN (object), *oldbin = GST_BIN (item->object);
  gdouble width, height, minwidth, minheight;

  if (oldbin&&gstbin) {
    g_print("gst_editor_bin_object_changed called with old object, fix this ******** code!!! Delete instead of simple hide\n ");
    //sleep(10);
    /* the proper solution would be to unref the child canvas items, but that
     * doesn't work right now (see bug 90259) -- just hide the children, then */

    for (l = bin->elements; l; l = l->next)
      goo_canvas_item_simple_hide (l->data);
    for (l = bin->links; l; l = l->next)
      goo_canvas_item_simple_hide (l->data);

    bin->elements = NULL;
    bin->links = NULL;
    //g_object_unref(G_OBJECT(oldbin));

    //normally done within realize, but otherwise it wont work:)
    g_signal_connect (gstbin, "element-added",
        G_CALLBACK (gst_editor_bin_element_added_cb), bin);
    g_signal_connect (gstbin, "element-removed",
        G_CALLBACK (gst_editor_bin_element_removed_cb), bin);
  }
  if (oldbin&&!gstbin) {//we are getting deleted
    g_print ("Disconnecting signals for old bin %s with pointer %p.\n",
        item->title_text, item);
    if (GST_IS_BIN (oldbin)) {
      g_signal_handlers_disconnect_by_func (oldbin,
          G_CALLBACK (gst_editor_bin_element_added_cb), bin);
      g_signal_handlers_disconnect_by_func (oldbin,
          G_CALLBACK (gst_editor_bin_element_removed_cb), bin);
    } else {
      g_print ("But GstBin seems to have disappeared\n");
    }
  }
  if (gstbin) {
    l = g_list_last(gstbin->children);

    if (l) {
      minwidth = pow (g_list_length (l), 0.75) * 125;
      minheight = pow (g_list_length (l), 0.25) * 125;

      g_object_get (item, "width", &width, "height", &height, NULL);
      if (minwidth > width + 1.0 && minheight > height + 1.0)
        g_object_set (item, "width", minwidth, "height", minheight, NULL);
    }

    while (l) {
      gst_editor_bin_element_added (GST_OBJECT (gstbin), GST_OBJECT (l->data),
          bin);
      l = g_list_previous (l);
    }
  }

  if (GST_EDITOR_ITEM_CLASS (parent_class)->object_changed)
    (GST_EDITOR_ITEM_CLASS (parent_class)->object_changed) (item, object);
}

/**********************************************************************
 * Callbacks from the gstbin (must be threadsafe)
 **********************************************************************/

static void
gst_editor_bin_element_added (GstObject * bin, GstObject * child,
    GstEditorBin * editorbin)
{
  g_print("Element %s with pointer %p Added to bin %s with pointer %p and editorbin pointer %p getting mutex...\n",GST_OBJECT_NAME(child),(void*)child,GST_OBJECT_NAME(bin),bin,editorbin);
  g_rw_lock_writer_lock (&(GST_EDITOR_ELEMENT(editorbin)->rwlock));
  GooCanvasItem *childitem;
  GstEditorItemAttr *attr = NULL;
  gdouble x, y, width, height;
  const gchar *child_name;

  child_name = GST_OBJECT_NAME (child);
  GST_DEBUG_OBJECT (bin, "adding new object %s, my attributes %p", child_name,
      editorbin->attributes);

  if (gst_editor_item_get (child)) {
    GST_DEBUG_OBJECT (bin, "child %s already rendered, ignoring", child_name);
    g_print("child %s already rendered, ignoring", child_name);
    GstEditorItem * realive=gst_editor_item_get (child);
    if (GOO_IS_CANVAS_ITEM(realive)) {g_print("seems to bee still alive, parent %p", goo_canvas_item_get_parent(GOO_CANVAS_ITEM(realive)));
       //if (!goo_canvas_item_get_parent(GOO_CANVAS_ITEM(realive))) goo_canvas_item_set_parent(GOO_CANVAS_ITEM(realive),GOO_CANVAS_ITEM (editorbin));
       //goo_canvas_item_raise(realive,GOO_CANVAS_ITEM (editorbin));
     }  
    g_rw_lock_writer_unlock (&(GST_EDITOR_ELEMENT(editorbin)->rwlock));
    return;
  }

  /* first check if we have previously stored position and size */
  if ((editorbin->attributes)&&(*editorbin->attributes)){
    GST_DEBUG_OBJECT (bin, "Trying to get attributes for %s, Attributes-Pointer %p, Attributes-Pointer-Pointer %p", child_name,*editorbin->attributes,editorbin->attributes);
    /* see if the datalist has coordinates for this element,
       and override if we do */
    attr = g_datalist_get_data (editorbin->attributes, child_name);
  }

  /* now decide */
  if (attr) {
    /* use loaded attributes */
    //if (GST_IS_BIN (child)) {
      x = attr->x;
      y = attr->y;
    //}
    //else{
    //  x = attr->x+1-GST_EDITOR_ITEM(editorbin)->l.w;//1 is box, rest is left border
    //  y = attr->y+1-GST_EDITOR_ITEM(editorbin)->t.h;
    //}
    width = attr->w;
    height = attr->h;
    g_datalist_remove_data (editorbin->attributes, child_name);
    GST_DEBUG_OBJECT (bin, "Got loaded attributes for %s", child_name);
  } else if (editorbin->element_x > 0) {
    /* first element to auto position */
    x = editorbin->element_x;
    y = editorbin->element_y;
    editorbin->element_x = -1;
    editorbin->element_y = -1;
    GST_DEBUG_OBJECT (bin, "got x/y based on element_x/_y %s", child_name);
  } else {
    /* autoposition element */
    gint len = g_list_length (editorbin->elements);

    g_object_get (editorbin, "width", &width, "height", &height, NULL);


    x = ((gint) (100 * len) % (gint) (width - 100)) + 15;
    y = ((gint) (100 * len) / (gint) (width - 100)) * 100 + 15;
    width=width/2;
    height=height/2;
    if (width < 150)
      width = 150;

    GST_DEBUG_OBJECT (bin, "# els: %d, x/y based on autopositioning for %s",
        len, child_name);

  }
  GST_DEBUG_OBJECT (bin, "putting child %s at x=%f; y=%f", child_name, x, y);

  if (gst_editor_bin_child_as_bin (editorbin, child)) {
    /* for bins, also pass along the attributes so they can also set
     * attributes on their children correctly */
    GST_DEBUG_OBJECT (bin, "child %s is a bin, setting attributes %p",
        child_name, editorbin->attributes);

    childitem = goo_canvas_item_new (GOO_CANVAS_ITEM (editorbin),
        gst_editor_bin_get_type (), "attributes", editorbin->attributes,
//        "object", child,
        "width", width, "height", height, "globallock",GST_EDITOR_ITEM(editorbin)->globallock,NULL);
    g_object_set (G_OBJECT (childitem), "object", child, NULL);
    gst_editor_bin_realize (childitem);
  } else {
    childitem = goo_canvas_item_new (GOO_CANVAS_ITEM (editorbin),
        gst_editor_element_get_type (), "object", child,
        "globallock",GST_EDITOR_ITEM(editorbin)->globallock,
        /* "width", width, "height", height, */ NULL);

    gst_editor_element_realize (childitem);

  }

#ifndef HAS_XY_PROP
//  goo_canvas_item_translate(childitem, x, y);
//  gst_editor_item_move (GST_EDITOR_ITEM (childitem), x,y);
goo_canvas_item_set_simple_transform (GOO_CANVAS_ITEM (childitem),x,y,1.,0.);
  /*g_print("Moved Item %f and Height %f, because it has been shifted by %f and %f\n",x,y,1-GST_EDITOR_ITEM(editorbin)->l.w,1-GST_EDITOR_ITEM(editorbin)->t.h);
gdouble getx;
gdouble gety;
gdouble getscale;
gdouble getrotation;
goo_canvas_item_get_simple_transform(GOO_CANVAS_ITEM (childitem),
&getx,
&gety,
&getscale,
&getrotation);

g_print("In editorbin, gotten Transformation x: %f y:%f Scale %f Rotation %f and X: %f Y:%f\n",getx,gety,getscale,getrotation,x,y);*/
#endif

  editorbin->elements = g_list_prepend (editorbin->elements, childitem);
  GST_DEBUG_OBJECT (bin, "done adding new object %s", child_name);
  g_object_ref (childitem);

  gst_editor_bin_repack (GST_EDITOR_ITEM (editorbin));
  //gst_editor_element_move (GST_EDITOR_ELEMENT (childitem), 0.0, 0.0);
  g_idle_add ((GSourceFunc) gst_editor_element_sync_state, editorbin);
  g_print("Finished adding element %s with pointer %p Added to bin %s with pointer %p getting mutex...List of parent has now %d entries\n",GST_OBJECT_NAME(child),(void*)child,GST_OBJECT_NAME(bin),bin,g_list_length(editorbin->elements));
  g_rw_lock_writer_unlock (&(GST_EDITOR_ELEMENT(editorbin)->rwlock));
}

/*
 * Can be called from another thread.
 * At least this seemed to be the case in GStreamer 0.10.
 */
static void
gst_editor_bin_element_added_cb (GstBin * bin, GstElement * child, gpointer user_data)
{
  GstEditorBin *editorbin = GST_EDITOR_BIN (user_data);

  g_debug ("gst_editor_bin_element_added_cb: %s with pointer %p Added to bin %s "
      "with pointer %p getting global mutex...",
      GST_OBJECT_NAME (child), (void *)child, GST_OBJECT_NAME (bin), bin);

  g_rw_lock_writer_lock (GST_EDITOR_ITEM (editorbin)->globallock);
  gst_editor_bin_element_added (GST_OBJECT (bin), GST_OBJECT (child), editorbin);
  g_rw_lock_writer_unlock (GST_EDITOR_ITEM (editorbin)->globallock);
}

static void
unset_deepdelete_editor_pads (GstEditorElement * element)
{
  int numc = g_list_length (element->srcpads);
  g_debug ("removing %d srcpads from element %p", numc, element);
  GList * l;
  l = element->srcpads;
  while (l) {
    if (GST_IS_EDITOR_ITEM (l->data)) {
      gst_editor_item_hash_remove (GST_EDITOR_ITEM (l->data)->object);
      g_object_set (GST_EDITOR_ITEM (l->data), "object", NULL,
          NULL);  // should free all the callbacks
    }
    if (GOO_IS_CANVAS_ITEM (l->data))
      goo_canvas_item_remove (l->data);
    l = g_list_next (l);
  }
  numc = g_list_length (element->sinkpads);
  g_debug ("removing %d sinkpads", numc);
  l = element->sinkpads;
  while (l) {
    if (GST_IS_EDITOR_ITEM (l->data)) {
      g_debug ("Processing sinkpad %p, object %p", l->data,
          GST_EDITOR_ITEM (l->data)->object);
      gst_editor_item_hash_remove (GST_EDITOR_ITEM (l->data)->object);
      g_object_set (GST_EDITOR_ITEM (l->data), "object", NULL,
          NULL);  // should free all the callbacks
    }
    g_debug ("deleting canvas next");
    if (GOO_IS_CANVAS_ITEM (l->data))
      goo_canvas_item_remove (l->data);
    l = g_list_next (l);
  }
  g_debug ("finished unset_deepdelete_editor_pads");
}

static void
unset_deepdelete_editor (GstEditorBin * bin)
{
  int numc = g_list_length (bin->elements);
  g_debug ("removing %d children", numc);
  GList * l;
  l = bin->elements;
  while (l) {
    if (GST_IS_EDITOR_BIN (l->data))
      unset_deepdelete_editor (l->data);
    if (GST_IS_EDITOR_ELEMENT (l->data))
      unset_deepdelete_editor_pads (GST_EDITOR_ELEMENT (l->data));
    if (GST_IS_EDITOR_ITEM (l->data)) {
      gst_editor_item_hash_remove (GST_EDITOR_ITEM (l->data)->object);
      gst_editor_item_disconnect (GST_EDITOR_ITEM (bin), l->data);
      // should free all the callbacks
      g_object_set (GST_EDITOR_ITEM (l->data), "object", NULL, NULL);
    }
    if (GOO_IS_CANVAS_ITEM (l->data))
      goo_canvas_item_remove (GOO_CANVAS_ITEM (l->data));
    l = g_list_next (l);
  }
  l = bin->links;
  while (l) {
    if ((l->data) && (GST_IS_EDITOR_LINK (l->data))) {
      if (GOO_IS_CANVAS_ITEM (GST_EDITOR_LINK (l->data)->canvas))
        goo_canvas_item_remove (
            GOO_CANVAS_ITEM (GST_EDITOR_LINK (l->data)->canvas));
      else
        g_debug ("GstEditorlink %p has no canvas", l->data);
      g_object_unref (l->data);
    } else
      g_debug ("failed to remove GstEditorlink %p", l->data);
    l = g_list_next (l);
  }
  g_debug ("unset_deepdelete finished");
}

static void
gst_editor_bin_element_removed_cb (GstBin * bin, GstElement * child, gpointer user_data)
{
  GstEditorBin *editorbin = GST_EDITOR_BIN (user_data);
  GstEditorElement *child_element;

  g_debug ("gst_editor_bin_element_removed_cb: %s with pointer %p removed from bin %s "
      "with pointer %p; getting global mutex...",
      GST_OBJECT_NAME (child), (void *)child, GST_OBJECT_NAME (bin), bin);

  g_rw_lock_writer_lock (GST_EDITOR_ITEM (editorbin)->globallock);
  child_element = GST_EDITOR_ELEMENT (gst_editor_item_get (GST_OBJECT (child)));
  g_assert (child_element != NULL);
  g_rw_lock_writer_lock (&child_element->rwlock);

  gst_editor_item_disconnect (GST_EDITOR_ITEM (editorbin),
      GST_EDITOR_ITEM (child_element));
  editorbin->elements = g_list_remove (editorbin->elements, child_element);

  if (child_element->active)
    g_object_set (goo_canvas_item_get_canvas (GOO_CANVAS_ITEM (child_element)),
        "selection", NULL, NULL);

  gst_editor_item_hash_remove (GST_OBJECT (child));

  if (GST_EDITOR_ITEM (child_element)->object)
    g_object_set (child_element, "object", NULL, NULL);

  /*
   * Kill all children (recursively).
   * This is necessary to properly clean up everything, update
   * the editor item hash, etc.
   * FIXME: It would be better to rely on the recursive invocation
   * of the element-removed signal on the bin. But it doesn't
   * get invoked when we call gst_editor_element_remove()
   * on the child bin's elements.
   */
  if (GST_IS_EDITOR_BIN (child_element))
    unset_deepdelete_editor (GST_EDITOR_BIN (child_element));

  goo_canvas_item_remove (GOO_CANVAS_ITEM (child_element));

  g_rw_lock_writer_unlock (&child_element->rwlock);
  g_rw_lock_writer_unlock (GST_EDITOR_ITEM(editorbin)->globallock);

  g_object_unref (child_element);

  g_debug ("Survived removing this item:%s %p Pointer of GstEditorElement %p",
      GST_OBJECT_NAME (child), child, child_element);
}

static gboolean
gst_editor_bin_child_as_bin (GstEditorBin * editorbin, GstObject * child)
{
  GstEditorCanvas *canvas =
      GST_EDITOR_CANVAS (goo_canvas_item_get_canvas (GOO_CANVAS_ITEM (editorbin)));

  /*
   * By default, we add only direct instances of GstBin or GstPipeline as
   * GstEditorBins to the canvas because other GstBin derivations are
   * usually self-managed and have factory-created children that we
   * shouldn't tamper with.
   */
  return G_OBJECT_TYPE (child) == GST_TYPE_BIN || G_OBJECT_TYPE (child) == GST_TYPE_PIPELINE ||
      (canvas->show_all_bins && GST_IS_BIN (child));
}

/**********************************************************************
 * Popup callbacks
 **********************************************************************/

static void
on_add_element (GSimpleAction * action,
    GVariant * parameter, gpointer user_data)
{
  GstEditorItem *item = GST_EDITOR_ITEM (user_data);
  GstElementFactory *factory;
  GstElement *element;

  if ((factory = gst_element_browser_pick_modal ())) {
    if (!(element = gst_element_factory_create (factory, NULL))) {
      g_warning ("unable to create element of type '%s'",
          GST_OBJECT_NAME (factory));
      return;
    }
    /* the element_added signal takes care of drawing the gui */
    gst_bin_add (GST_BIN (item->object), element);
  }
}

static void
on_paste (GSimpleAction * action,
    GVariant * parameter, gpointer user_data)
{
  GstEditorBin *bin = GST_EDITOR_BIN (user_data);

  gst_editor_bin_paste (bin, GDK_SELECTION_CLIPBOARD);
}

/**********************************************************************
 * Public functions
 **********************************************************************/

/* a convenience structure */
typedef struct _element element;

struct _element
{
  GstEditorElement *element;
  gdouble x;
  gdouble y;
  gdouble w;
  gdouble h;
  gdouble fx;
  gdouble fy;
};

/* links are ideally 20 pixels long and horizontal. The force is directly
   proportional to the 'stretching' of the links. */
static void
calculate_link_forces (GList * links)
{
  GList *l;
  GstEditorElement *src, *sink;
  GstEditorLink *c;
  element *srce, *sinke;
  gdouble x1, x2, y1, y2, fx, fy;

  for (l = links; l; l = l->next) {
    c = GST_EDITOR_LINK (l->data);
    src =
        GST_EDITOR_ELEMENT (goo_canvas_item_get_parent (GOO_CANVAS_ITEM (c->
                srcpad)));
    sink =
        GST_EDITOR_ELEMENT (goo_canvas_item_get_parent (GOO_CANVAS_ITEM (c->
                sinkpad)));


    srce = g_object_get_data (G_OBJECT (src), "sort-data");
    sinke = g_object_get_data (G_OBJECT (sink), "sort-data");

    g_object_get (c, "x1", &x1, "y1", &y1, "x2", &x2, "y2", &y2, NULL);

    fx = (x2 - x1 - 20) * 0.5;
    fy = (y2 - y1) * 0.5;

/*
    len = sqrt ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    if (len-20.0 < 0) {
      fx *= 2.0;
      fy *= 2.0;
    }
*/

    if (srce) {
      srce->fx += fx;
      srce->fy += fy;
    }
    if (sinke) {
      sinke->fx -= fx;
      sinke->fy -= fy;
    }
  }
}

/* elements repel each other in direct proportion to the degree that they
   overlap in the x and y directions. This repulsion starts when elements get
   closer than 15 pixels away in the x direction, and 5 in the y direction. */
static void
calculate_element_repulsion_forces (element * e, gint num_children)
{
  gint i, j;
  gdouble fx, fy;
  gdouble x1, y1, x2, y2;

  /* discourage element overlap */
  for (i = 0; i < num_children; i++) {
    for (j = i + 1; j < num_children; j++) {
      /* we want the coordinates of the centers of the elements */
      x1 = e[i].x + e[i].w * 0.5;
      x2 = e[j].x + e[j].w * 0.5;
      y1 = e[i].y + e[i].h * 0.5;
      y2 = e[j].y + e[j].h * 0.5;

      /* x distance more important than y distance */
      fx = (0.5 * (e[i].w + e[j].w) + 15 - ABS (x2 - x1)) * 1.5;
      fy = (0.5 * (e[i].h + e[j].h) + 5 - ABS (y2 - y1)) * 1.5;

      if (fx > 0 && fy > 0) {
        e[i].fx += fx * ((x1 > x2) ? 1.0 : -1.0);
        e[j].fx += fx * ((x1 > x2) ? -1.0 : 1.0);
        e[i].fy += fy * ((y1 > y2) ? 1.0 : -1.0);
        e[j].fy += fy * ((y1 > y2) ? -1.0 : 1.0);
      }
    }
  }
}

gdouble
gst_editor_bin_sort (GstEditorBin * bin, gdouble step)
{
  gdouble ret = 0;
  gint i, num_children;
  GstEditorElement *gstelement;
  element *e;
  GList *l;

  g_return_val_if_fail (GST_IS_EDITOR_BIN (bin), 0);

  num_children = g_list_length (bin->elements);
  if (num_children == 0)
    return 0;

  /* set up the element* structs */
  e = g_new0 (element, num_children);
  for (l = bin->elements, i = 0; l; l = l->next, i++) {
    gstelement = GST_EDITOR_ELEMENT (l->data);
    g_object_get (gstelement, "x", &e[i].x, "y", &e[i].y, "width", &e[i].w,
        "height", &e[i].h, NULL);



    e[i].element = gstelement;
    g_object_set_data (G_OBJECT (gstelement), "sort-data", &e[i]);
  }

  /* calculate the forces */
  calculate_link_forces (bin->links);
  calculate_element_repulsion_forces (e, num_children);

  /* do the moving */
  for (i = 0; i < num_children; i++) {
    gst_editor_element_move (e[i].element, e[i].fx * step, e[i].fy * step);
    g_object_set_data (G_OBJECT (e[i].element), "sort-data", NULL);

    ret += ABS (e[i].fx) * step + ABS (e[i].fy) * step;

    if (GST_IS_EDITOR_BIN (e[i].element))
      ret += gst_editor_bin_sort ((GstEditorBin *) e[i].element, step);
  }

  g_free (e);

  return ret;
}

gboolean
gst_editor_bin_paste_from_string (GstEditorBin * bin, const gchar * str,
    GError ** parent_error)
{
  GError *error = NULL;
  GstElement *element;

  /*
   * gst_editor_element_copy() will only copy single-element
   * pipelines into the clipboard.
   * However, the user could have copied a multi-element
   * pipeline from somewhere - in this case, it is pasted as
   * a GstBin, so links can be preserved.
   * NOTE: We do non-strict parsing here, so gst_parse_launch()
   * can set `error` even if it returns an element.
   */
  element = gst_parse_launch_full (str, NULL,
      GST_PARSE_FLAG_NO_SINGLE_ELEMENT_BINS, &error);
  if (!element) {
    g_propagate_error (parent_error, error);
    g_prefix_error (parent_error, "Could not paste: ");
    return FALSE;
  }
  if (error) {
    g_warning ("Parsing issue during paste: %s", error->message);
    g_clear_error (&error);
  }

  /* `element` has a floating reference */
  gst_bin_add (GST_BIN (GST_EDITOR_ITEM (bin)->object), element);
  return TRUE;
}

void
gst_editor_bin_paste (GstEditorBin * bin, GdkAtom selection)
{
  GtkClipboard *clipboard = gtk_clipboard_get (selection);
  gchar *text;
  GError *error = NULL;

  g_return_if_fail (GST_IS_EDITOR_BIN (bin));

  /*
   * GstEditorElements are saved as plain-text pipelines currently,
   * See gst_editor_element_copy().
   */
  text = gtk_clipboard_wait_for_text (clipboard);
  if (!text) {
    g_object_set (goo_canvas_item_get_canvas (GOO_CANVAS_ITEM (bin)), "status",
        "Could not paste: Empty clipboard.", NULL);
    return;
  }

  if (!gst_editor_bin_paste_from_string (bin, text, &error)) {
    g_free (text);
    g_object_set (goo_canvas_item_get_canvas (GOO_CANVAS_ITEM (bin)), "status",
        error->message, NULL);
    g_error_free (error);
    return;
  }

  g_free (text);
}

void
gst_editor_bin_debug_output (GstEditorBin * bin){
  const GList *children;
  GstEditorItem * item;
  GstEditorElement * element;
  GooCanvasItem * canvas;
  item=GST_EDITOR_ITEM(bin);
  element=GST_EDITOR_ELEMENT(bin);
  canvas=GOO_CANVAS_ITEM (bin);
  //local informations
  if (!bin){
    g_print("Selection is NULL");     
    return;
  }
  g_print("Information about GstEditorBin object %p:",bin);
  g_print("GstEditorItem: Titletext %s    \n",item->title_text);
  g_print("GstEditorElement: sinks: %d sources: %d \n",element->sinks,element->srcs);
  if (!GST_IS_EDITOR_BIN(bin)) return;
  g_print("GstEditorBin: elements: %d links: %d. Getting children from EditorBin.\n",g_list_length(bin->elements),g_list_length(bin->links));
  //iterate children of editorbin
  const GList *elements;
  elements=bin->elements;
  while (elements) {
  if (GST_IS_EDITOR_BIN(elements->data)){
    gst_editor_bin_debug_output(GST_EDITOR_BIN(elements->data));
    }
  else if (GST_IS_EDITOR_ELEMENT(elements->data)){
    g_print("Child Element of gstbin is called %s",GST_EDITOR_ITEM(elements->data)->title_text);
    }
  elements = g_list_next (elements);
  }
  //iterare children of goo_canvas_item
  g_print("canvas Item has children: %d\n",goo_canvas_item_get_n_children(canvas));
  //iterate children of GstObject
  children = g_list_last(GST_BIN_CHILDREN (GST_BIN (item->object)));
  g_print("Getting Gstreamer Children, Total %d\n", g_list_length(GST_BIN_CHILDREN (GST_BIN (item->object))));
  while (children) {
    if (GST_IS_BIN(children->data)){
      GstEditorBin * childbin;
      childbin=(GST_EDITOR_BIN(gst_editor_item_get(children->data)));
      if (childbin) gst_editor_bin_debug_output(childbin);
    }
    else if (GST_IS_ELEMENT(children->data)){
      g_print("Found Gstreamer Element:%s\n",GST_OBJECT_NAME(children->data) );
    }
    else 
      g_print("unknown child type \n");
    children = g_list_previous (children);
  }
}

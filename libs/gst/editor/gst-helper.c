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
    
#include <goocanvas.h>
#include "gst-helper.h"
void 
gsth_element_unlink_all (GstElement * element) 
{
  gboolean done = FALSE;
  GstIterator * it;
  it = gst_element_iterate_pads (element);
  while (!done)
     {
    gpointer item;
    switch (gst_iterator_next (it, &item))
       {
      case GST_ITERATOR_OK:
       {
        GstPad * pad = GST_PAD (item);
        GstPad * peer = GST_PAD_PEER (pad);
        if (peer)
          if (GST_PAD_IS_SRC(pad)) gst_pad_unlink (pad, peer);
else gst_pad_unlink (peer,pad);
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


/**
 * goo_canvas_item_show:
 * @item: A canvas item.
 *
 * Shows a canvas item.  If the item was already shown, then no action is taken.
 **/ 
void 
goo_canvas_item_simple_show (GooCanvasItemSimple * item) 
{
  g_return_if_fail (GOO_IS_CANVAS_ITEM (item));
  if (!goo_canvas_item_is_visible (GOO_CANVAS_ITEM (item)))
     {
    g_object_set (item, "visibility", GOO_CANVAS_ITEM_VISIBLE, NULL);
    goo_canvas_request_redraw (item->canvas, &item->bounds);
    }
}


/**
 * goo_canvas_item_hide:
 * @item: A canvas item.
 *
 * Hides a canvas item.  If the item was already hidden, then no action is
 * taken.
 **/ 
void 
goo_canvas_item_simple_hide (GooCanvasItemSimple * item) 
{
  g_return_if_fail (GOO_IS_CANVAS_ITEM (item));
  if (goo_canvas_item_is_visible (GOO_CANVAS_ITEM (item)))
     {
    g_object_set (item, "visibility", GOO_CANVAS_ITEM_INVISIBLE, NULL);        // GOO_CANVAS_ITEM_HIDDEN ??
    if (item->canvas)
       {
      goo_canvas_request_redraw (item->canvas, &item->bounds);
      }
    }
}


/**
 * goo_canvas_item_new:
 * @parent: The parent group for the new item.
 * @type: The object type of the item.
 * @first_arg_name: A list of object argument name/value pairs, NULL-terminated,
 * used to configure the item.  For example, "fill_color", "black",
 * "width_units", 5.0, NULL.
 * @Varargs:
 *
 * Creates a new canvas item with @parent as its parent group.  The item is
 * created at the top of its parent's stack, and starts up as visible.  The item
 * is of the specified @type, for example, it can be
 * gnome_canvas_rect_get_type().  The list of object arguments/value pairs is
 * used to configure the item.
 *
 * Return value: The newly-created item.
 **/ 
    GooCanvasItem * goo_canvas_item_new (GooCanvasItem * parent, GType type,
    const gchar * first_arg_name, ...) 
{
  GooCanvasItem * item;
  va_list args;
  g_return_val_if_fail (GOO_IS_CANVAS_GROUP (parent), NULL);
  g_return_val_if_fail (g_type_is_a (type, goo_canvas_item_get_type ()), NULL);
  item = GOO_CANVAS_ITEM (g_object_new (type, NULL));
  va_start (args, first_arg_name);
  if (first_arg_name)
    g_object_set_valist (G_OBJECT (item), first_arg_name, args);
  va_end (args);
  if (parent)
     {
    goo_canvas_item_add_child (parent, item, -1);
    g_object_unref (item);
    }
  return item;
}


/**
 * goo_canvas_item_raise:
 * @item: A canvas item.
 * @positions: Number of steps to raise the item.
 *
 * Raises the item in its parent's stack by the specified number of positions.
 * If the number of positions is greater than the distance to the top of the
 * stack, then the item is put at the top.
 **/ 
void 
goo_canvas_item_raise_pos (GooCanvasItem * item, int positions) 
{
  gint old_position, new_position;
  GooCanvasItem * parent;
  g_return_if_fail (GOO_IS_CANVAS_ITEM (item));
  g_return_if_fail (positions >= 0);
  parent = goo_canvas_item_get_parent (item);
  if (!parent || positions == 0)
    return;
  old_position =
      goo_canvas_item_find_child (goo_canvas_item_get_parent (item), item);
  new_position = (old_position < positions ? 0 : old_position - positions);
  goo_canvas_item_move_child (item, old_position, new_position);
}


/**
 * goo_canvas_item_lower:
 * @item: A canvas item.
 * @positions: Number of steps to lower the item.
 *
 * Lowers the item in its parent's stack by the specified number of positions.
 * If the number of positions is greater than the distance to the bottom of the
 * stack, then the item is put at the bottom.
 **/ 
void 
goo_canvas_item_lower_pos (GooCanvasItem * item, int positions) 
{
  gint old_position, new_position;
  gint length;
  length = goo_canvas_item_get_n_children (goo_canvas_item_get_parent (item));
  old_position =
      goo_canvas_item_find_child (goo_canvas_item_get_parent (item), item);
  new_position =
      (old_position + positions >=
      length ? length - 1 : old_position + positions);
  goo_canvas_item_move_child (item, old_position, new_position);
} 

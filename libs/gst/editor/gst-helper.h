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


#ifndef __GST_HELPER_H__
#define __GST_HELPER_H__

#include <gst/gst.h>
#include <goocanvas.h>

void gsth_element_unlink_all (GstElement * element);
void goo_canvas_item_simple_show (GooCanvasItemSimple *item);
void goo_canvas_item_simple_hide (GooCanvasItemSimple *item);
GooCanvasItem *goo_canvas_item_new (GooCanvasItem *parent,
    GType type, const gchar *first_arg_name, ...);
//void goo_canvas_item_raise_pos (GooCanvasItem *item, int positions);
//void goo_canvas_item_lower_pos (GooCanvasItem *item, int positions);

#endif /* __GST_HELPER_H__ */

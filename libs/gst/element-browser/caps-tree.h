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

#include <gst/gst.h>
#include <gtk/gtk.h>


#ifndef __GST_ELEMENT_BROWSER_CAPS_TREE_H__
#define __GST_ELEMENT_BROWSER_CAPS_TREE_H__


#define GST_TYPE_ELEMENT_BROWSER_CAPS_TREE           (gst_element_browser_caps_tree_get_type())
#define GST_ELEMENT_BROWSER_CAPS_TREE(obj)           (GTK_CHECK_CAST ((obj), GST_TYPE_ELEMENT_BROWSER_CAPS_TREE, GstElementBrowserCapsTree))
#define GST_ELEMENT_BROWSER_CAPS_TREE_CLASS(klass)   (GTK_CHECK_CLASS_CAST ((klass), GST_TYPE_ELEMENT_BROWSER_CAPS_TREE, GstElementBrowserCapsTreeClass))
#define GST_IS_ELEMENT_BROWSER_CAPS_TREE(obj)        (GTK_CHECK_TYPE ((obj), GST_TYPE_ELEMENT_BROWSER_CAPS_TREE))
#define GST_IS_ELEMENT_BROWSER_CAPS_TREE_CLASS(obj)  (GTK_CHECK_CLASS_TYPE ((klass), GST_TYPE_ELEMENT_BROWSER_CAPS_TREE))


typedef struct _GstElementBrowserCapsTree GstElementBrowserCapsTree;
typedef struct _GstElementBrowserCapsTreeClass GstElementBrowserCapsTreeClass;


struct _GstElementBrowserCapsTree
{
  GtkScrolledWindow parent;

  GtkWidget *treeview;
  GtkTreeStore *store;

  GstElementFactory *factory;
  GstElement *element;
};

struct _GstElementBrowserCapsTreeClass
{
  GtkScrolledWindowClass parent_class;
};


GType gst_element_browser_caps_tree_get_type (void);
GtkWidget *gst_element_browser_caps_tree_new (void);


#endif /* __GST_ELEMENT_BROWSER_CAPS_TREE_H__ */

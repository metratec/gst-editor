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


#ifndef __GST_ELEMENT_BROWSER_ELEMENT_TREE_H__
#define __GST_ELEMENT_BROWSER_ELEMENT_TREE_H__


#define GST_TYPE_ELEMENT_BROWSER_ELEMENT_TREE           (gst_element_browser_element_tree_get_type())
#define GST_ELEMENT_BROWSER_ELEMENT_TREE(obj)           (GTK_CHECK_CAST ((obj), GST_TYPE_ELEMENT_BROWSER_ELEMENT_TREE, GstElementBrowserElementTree))
#define GST_ELEMENT_BROWSER_ELEMENT_TREE_CLASS(klass)   (GTK_CHECK_CLASS_CAST ((klass), GST_TYPE_ELEMENT_BROWSER_ELEMENT_TREE, GstElementBrowserElementTreeClass))
#define GST_IS_ELEMENT_BROWSER_ELEMENT_TREE(obj)        (GTK_CHECK_TYPE ((obj), GST_TYPE_ELEMENT_BROWSER_ELEMENT_TREE))
#define GST_IS_ELEMENT_BROWSER_ELEMENT_TREE_CLASS(obj)  (GTK_CHECK_CLASS_TYPE ((klass), GST_TYPE_ELEMENT_BROWSER_ELEMENT_TREE))


typedef struct _GstElementBrowserElementTree GstElementBrowserElementTree;
typedef struct _GstElementBrowserElementTreeClass
    GstElementBrowserElementTreeClass;


struct _GstElementBrowserElementTree
{
  GtkBin parent;

  GtkWidget *view;
  GtkTreeStore *store;
  GtkListStore *filter_store;
  GtkTreeModel *cur_model;
  GtkEntry *filter_entry;

  GstElementFactory *selected;

  gchar *current_filter_text;
  guint filter_idle_id;
};

struct _GstElementBrowserElementTreeClass
{
  GtkBinClass parent_class;
  void (* selected_sig)  (GstElementBrowserElementTree *browser);
  void (* activated_sig)  (GstElementBrowserElementTree *browser);
};


GType gst_element_browser_element_tree_get_type (void);
GtkWidget *gst_element_browser_element_tree_new (void);

#endif /* __GST_ELEMENT_BROWSER_ELEMENT_TREE_H__ */

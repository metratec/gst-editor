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

#include "caps-tree.h"


#ifndef __GST_ELEMENT_BROWSER_H__
#define __GST_ELEMENT_BROWSER_H__


#define GST_TYPE_ELEMENT_BROWSER           (gst_element_browser_get_type())
#define GST_ELEMENT_BROWSER(obj)           (GTK_CHECK_CAST ((obj), GST_TYPE_ELEMENT_BROWSER, GstElementBrowser))
#define GST_ELEMENT_BROWSER_CLASS(klass)   (GTK_CHECK_CLASS_CAST ((klass), GST_TYPE_ELEMENT_BROWSER, GstElementBrowserClass))
#define GST_IS_ELEMENT_BROWSER(obj)        (GTK_CHECK_TYPE ((obj), GST_TYPE_ELEMENT_BROWSER))
#define GST_IS_ELEMENT_BROWSER_CLASS(obj)  (GTK_CHECK_CLASS_TYPE ((klass), GST_TYPE_ELEMENT_BROWSER))


typedef struct _GstElementBrowser GstElementBrowser;
typedef struct _GstElementBrowserClass GstElementBrowserClass;


struct _GstElementBrowser
{
  GtkDialog dialog;

  GstElementFactory *selected;
  GstElement *element;

  GtkWidget *longname;
  GtkWidget *description;
  GtkWidget *author;

  GtkWidget *pads;
  GtkWidget *padtemplates;
};

struct _GstElementBrowserClass
{
  GtkDialogClass parent_class;
};


GType gst_element_browser_get_type (void);
GtkWidget *gst_element_browser_new (void);
GstElementFactory *gst_element_browser_pick_modal (void);


#endif /* __GST_ELEMENT_BROWSER_H__ */

/* -*- Mode: C; c-basic-offset: 4 -*- */
/* GStreamer Element View and Controller
 * Copyright (C) <2002> Andy Wingo <wingo@pobox.com>
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

#include <gtk/gtk.h>
#include <gst/gst.h>

#ifndef __GST_ELEMENT_UI_PROP_VIEW_H__
#define __GST_ELEMENT_UI_PROP_VIEW_H__

#define GST_TYPE_ELEMENT_UI_PROP_VIEW           (gst_element_ui_prop_view_get_type())
#define GST_ELEMENT_UI_PROP_VIEW(obj)           (GTK_CHECK_CAST ((obj), GST_TYPE_ELEMENT_UI_PROP_VIEW, GstElementUIPropView))
#define GST_ELEMENT_UI_PROP_VIEW_CLASS(klass)   (GTK_CHECK_CLASS_CAST ((klass), GST_TYPE_ELEMENT_UI_PROP_VIEW, GstElementUIPropViewClass))
#define GST_IS_ELEMENT_UI_PROP_VIEW(obj)        (GTK_CHECK_TYPE ((obj), GST_TYPE_ELEMENT_UI_PROP_VIEW))
#define GST_IS_ELEMENT_UI_PROP_VIEW_CLASS(obj)  (GTK_CHECK_CLASS_TYPE ((klass), GST_TYPE_ELEMENT_UI_PROP_VIEW))

typedef struct _GstElementUIPropView GstElementUIPropView;
typedef struct _GstElementUIPropViewClass GstElementUIPropViewClass;

struct _GstElementUIPropView
{
  GtkVBox vbox;

  GstElement *element;		/* element we're viewing properties of */
  GParamSpec *param;

  GValue *value;
  GMutex *value_mutex;
  guint source_id;		/* event source id for the update timeout */

  gboolean on_pending;
  gboolean on_set;
  GtkObject *adjustment;
  GtkWidget *optionmenu;
  gint *enum_values;
  gpointer *enum_pointer;
  GtkWidget *label_lower;
  GtkWidget *spinbutton;
  GtkWidget *toggle_on;
  GtkWidget *toggle_off;
  GtkWidget *entry;
  GtkWidget *label_upper;
  GtkWidget *hscale;
  GtkWidget *file;
  GtkWidget *filetext;
};

struct _GstElementUIPropViewClass
{
  GtkVBoxClass parent_class;
};

GType gst_element_ui_prop_view_get_type ();
gboolean gst_element_ui_prop_view_update (GstElementUIPropView * pview);
gboolean gst_element_ui_prop_view_update_async (GstElementUIPropView * pview);

#endif /* __GST_ELEMENT_UI_PROP_VIEW_H__ */

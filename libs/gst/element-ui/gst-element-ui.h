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

#include "gst-element-ui-prop-view.h"

#ifndef __GST_ELEMENT_UI_H__
#define __GST_ELEMENT_UI_H__

#define GST_TYPE_ELEMENT_UI           (gst_element_ui_get_type())
#define GST_ELEMENT_UI(obj)           (GTK_CHECK_CAST ((obj), GST_TYPE_ELEMENT_UI, GstElementUI))
#define GST_ELEMENT_UI_CLASS(klass)   (GTK_CHECK_CLASS_CAST ((klass), GST_TYPE_ELEMENT_UI, GstElementUIClass))
#define GST_IS_ELEMENT_UI(obj)        (GTK_CHECK_TYPE ((obj), GST_TYPE_ELEMENT_UI))
#define GST_IS_ELEMENT_UI_CLASS(obj)  (GTK_CHECK_CLASS_TYPE ((klass), GST_TYPE_ELEMENT_UI))

typedef enum
{
  GST_ELEMENT_UI_VIEW_MODE_COMPACT = 1,
  GST_ELEMENT_UI_VIEW_MODE_FULL
} GstElementUIViewMode;

typedef struct _GstElementUI GstElementUI;
typedef struct _GstElementUIClass GstElementUIClass;

struct _GstElementUI
{
  GtkTable table;

  GstElement *element;

  GstElementUIViewMode view_mode;
  gboolean show_readonly;
  gboolean show_writeonly;
  gchar *exclude_string;

  gint nprops;
  gint selected;
  GParamSpec **params;
  GstElementUIPropView **pviews;
  GtkWidget **plabels;

  gboolean active;

  GtkWidget *name;
  GtkWidget *optionmenu;
  GtkWidget *message;
};

struct _GstElementUIClass
{
  GtkTableClass parent_class;
};

GType gst_element_ui_get_type ();
GstElementUI *gst_element_ui_new (GstElement * element);

#endif /* __GST_ELEMENT_UI_H__ */

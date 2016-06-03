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


#ifndef __GST_EDITOR_PAD_H__
#define __GST_EDITOR_PAD_H__

#include <goocanvas.h>
#include <gst/gst.h>
#include <gst/editor/editor.h>


#define GST_EDITOR_PAD(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, GST_TYPE_EDITOR_PAD, GstEditorPad)
#define GST_EDITOR_PAD_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, GST_TYPE_EDITOR_PAD, GstEditorPadClass)
#define GST_IS_EDITOR_PAD(obj) G_TYPE_CHECK_INSTANCE_TYPE(obj, GST_TYPE_EDITOR_PAD)
#define GST_IS_EDITOR_PAD_CLASS(klass) G_TYPE_CHECK_CLASS_TYPE(klass, GST_TYPE_EDITOR_PAD)
#define GST_TYPE_EDITOR_PAD gst_editor_pad_get_type()

#define GST_EDITOR_PAD_SOMETIMES(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, GST_TYPE_EDITOR_PAD_SOMETIMES, GstEditorPadSometimes)
#define GST_EDITOR_PAD_SOMETIMES_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, GST_TYPE_EDITOR_PAD_SOMETIMES, GstEditorPadSometimesClass)
#define GST_IS_EDITOR_PAD_SOMETIMES(obj) G_TYPE_CHECK_INSTANCE_TYPE(obj, GST_TYPE_EDITOR_PAD_SOMETIMES)
#define GST_IS_EDITOR_PAD_SOMETIMES_CLASS(klass) G_TYPE_CHECK_CLASS_TYPE(klass, GST_TYPE_EDITOR_PAD_SOMETIMES)
#define GST_TYPE_EDITOR_PAD_SOMETIMES gst_editor_pad_sometimes_get_type()

#define GST_EDITOR_PAD_ALWAYS(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, GST_TYPE_EDITOR_PAD_ALWAYS, GstEditorPadAlways)
#define GST_EDITOR_PAD_ALWAYS_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, GST_TYPE_EDITOR_PAD_ALWAYS, GstEditorPadAlwaysClass)
#define GST_IS_EDITOR_PAD_ALWAYS(obj) G_TYPE_CHECK_INSTANCE_TYPE(obj, GST_TYPE_EDITOR_PAD_ALWAYS)
#define GST_IS_EDITOR_PAD_ALWAYS_CLASS(klass) G_TYPE_CHECK_CLASS_TYPE(klass, GST_TYPE_EDITOR_PAD_ALWAYS)
#define GST_TYPE_EDITOR_PAD_ALWAYS gst_editor_pad_always_get_type()

#define GST_EDITOR_PAD_REQUEST(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, GST_TYPE_EDITOR_PAD_REQUEST, GstEditorPadRequest)
#define GST_EDITOR_PAD_REQUEST_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, GST_TYPE_EDITOR_PAD_REQUEST, GstEditorPadRequestClass)
#define GST_IS_EDITOR_PAD_REQUEST(obj) G_TYPE_CHECK_INSTANCE_TYPE(obj, GST_TYPE_EDITOR_PAD_REQUEST)
#define GST_IS_EDITOR_PAD_REQUEST_CLASS(klass) G_TYPE_CHECK_CLASS_TYPE(klass, GST_TYPE_EDITOR_PAD_REQUEST)
#define GST_TYPE_EDITOR_PAD_REQUEST gst_editor_pad_request_get_type()

#define GST_EDITOR_PAD_REQUESTED(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, GST_TYPE_EDITOR_PAD_REQUESTED, GstEditorPadRequested)
#define GST_EDITOR_PAD_REQUESTED_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, GST_TYPE_EDITOR_PAD_REQUESTED, GstEditorPadRequestedClass)
#define GST_IS_EDITOR_PAD_REQUESTED(obj) G_TYPE_CHECK_INSTANCE_TYPE(obj, GST_TYPE_EDITOR_PAD_REQUESTED)
#define GST_IS_EDITOR_PAD_REQUESTED_CLASS(klass) G_TYPE_CHECK_CLASS_TYPE(klass, GST_TYPE_EDITOR_PAD_REQUESTED)
#define GST_TYPE_EDITOR_PAD_REQUESTED gst_editor_pad_requested_get_type()

#define GST_EDITOR_PAD_GHOST(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, GST_TYPE_EDITOR_PAD_GHOST, GstEditorPadGhost)
#define GST_EDITOR_PAD_GHOST_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, GST_TYPE_EDITOR_PAD_GHOST, GstEditorPadGhostClass)
#define GST_IS_EDITOR_PAD_GHOST(obj) G_TYPE_CHECK_INSTANCE_TYPE(obj, GST_TYPE_EDITOR_PAD_GHOST)
#define GST_IS_EDITOR_PAD_GHOST_CLASS(klass) G_TYPE_CHECK_CLASS_TYPE(klass, GST_TYPE_EDITOR_PAD_GHOST)
#define GST_TYPE_EDITOR_PAD_GHOST gst_editor_pad_ghost_get_type()


struct _GstEditorPad
{
  GstEditorItem item;

  /* convenience */
  gboolean issrc;
  gboolean istemplate;
  gboolean isghost;
  GstPadPresence presence;

  /* links */
  GstEditorLink *link;
  GstEditorLink *ghostlink;

  /* the link boxes */
  GooCanvasItem *srcbox;
  GooCanvasItem *sinkbox;

  /* interaction state */
  gboolean dragging, resizing, moved;
  gdouble dragx, dragy;

  gboolean linking;
  gboolean unlinking;
};

struct _GstEditorPadClass
{
  GstEditorItemClass parent_class;
};


GType gst_editor_pad_get_type (void);
GType gst_editor_pad_always_get_type (void);
GType gst_editor_pad_sometimes_get_type (void);
GType gst_editor_pad_request_get_type (void);
GType gst_editor_pad_requested_get_type (void);
GType gst_editor_pad_ghost_get_type (void);

/*
 * FIXME: It shouldn't be necessary to export realize
 * handlers.
 */
void gst_editor_pad_realize (GooCanvasItem * citem);

#endif /* __GST_EDITOR_PAD_H__ */

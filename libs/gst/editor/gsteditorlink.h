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


#ifndef __GST_EDITOR_LINK_H__
#define __GST_EDITOR_LINK_H__

#include <goocanvas.h>
#include <gst/editor/gsteditoritem.h>

#define GST_TYPE_EDITOR_LINK (gst_editor_link_get_type())
#define GST_EDITOR_LINK(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_EDITOR_LINK, GstEditorLink))
#define GST_EDITOR_LINK_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_EDITOR_LINK, GstEditorLinkClass))
#define GST_IS_EDITOR_LINK(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_EDITOR_LINK))
#define GST_IS_EDITOR_LINK_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_EDITOR_LINK))
#define GST_EDITOR_LINK_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_EDITOR_LINK, GstEditorLinkClass))


struct _GstEditorLink
{
  GooCanvasPolyline line;

  /* the two pads we're linking */
  GstEditorItem *srcpad, *sinkpad;

  /* toplevel canvas */
  GstEditorCanvas *canvas;

  /* whether we've been realized or not */
  gboolean realized;

  /* are we a ghosted link? */
  gboolean ghost;

  /* are we a ghosted link? */
  gboolean dragging;

  /* visual stuff */
  GooCanvasPoints *points;

  gdouble x, y;			/* terminating point    */
};

struct _GstEditorLinkClass
{
   GooCanvasPolylineClass parent_class;
};


GType gst_editor_link_get_type (void);
gboolean gst_editor_link_link (GstEditorLink * link);
void gst_editor_link_destroy(GstEditorLink * link);
void gst_editor_link_unlink (GstEditorLink * link);


#endif /* __GST_EDITOR_LINK_H__ */

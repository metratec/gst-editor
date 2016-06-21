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



#ifndef __GST_EDITOR_ELEMENT_H__
#define __GST_EDITOR_ELEMENT_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <goocanvas.h>

#include <gst/editor/editor.h>
#include <gst/editor/gsteditoritem.h>


#define GST_TYPE_EDITOR_ELEMENT (gst_editor_element_get_type())
#define GST_EDITOR_ELEMENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_EDITOR_ELEMENT, GstEditorElement))
#define GST_EDITOR_ELEMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_EDITOR_ELEMENT, GstEditorElementClass))
#define GST_IS_EDITOR_ELEMENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_EDITOR_ELEMENT))
#define GST_IS_EDITOR_ELEMENT_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_EDITOR_ELEMENT))
#define GST_EDITOR_ELEMENT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_EDITOR_ELEMENT, GstEditorElementClass))

#define GST_EDITOR_ELEMENT_PARENT(obj) (GST_EDITOR_ELEMENT(obj)->parent)
#define GST_EDITOR_ELEMENT_GROUP(obj) (GST_EDITOR_ELEMENT(obj)->group)
#define GST_EDITOR_ELEMENT_CANVAS(obj) (GST_EDITOR_ELEMENT(obj)->canvas)


struct _GstEditorElement
{
  GstEditorItem item;

  /*
   * The parent of the GstElement associated with this
   * GstEditorElement.
   * We need to track this so we can handle parent unsetting.
   */
  GstObject *parent_object;

  GooCanvasItem *resizebox;	/* easy ones */
  GooCanvasItem *statebox;	/* the box around the selected
				   state */
  GooCanvasItem *stateicons[4];	/* element state icons */

  gdouble insidewidth, insideheight;	/* minimum space inside */
  gdouble titlewidth, titleheight;	/* size of title */
  gdouble statewidth, stateheight;	/* size of state boxes */
  gdouble sinkwidth, sinkheight;	/* size of sink pads */
  gdouble srcwidth, srcheight;	/* size of src pads */
  gint sinks, srcs;		/* how many pads? */

  GooCanvasGroup *contents;	/* contents if any */

  gboolean active;		/* is it active (currently selected) */
  gboolean resizeable;
  gboolean moveable;

  GList *srcpads, *sinkpads;	/* list of pads */
  gboolean padlistchange;

  guint source;			/* the GSource id for gst_bin_iterate */

  gboolean dragging, resizing, moved, hesitating;	/* interaction state */

  gdouble offx, offy, dragx, dragy;

  guint set_state_idle_id;
  GstState next_state;

  guint bus_id;
  GRWLock rwlock; 
};

struct _GstEditorElementClass
{
  GstEditorItemClass parent_class;

  void (*position_changed) (GstEditorElement * element);
  void (*size_changed) (GstEditorElement * element);
//gint (*event) (GnomeCanvasItem * item, GdkEvent * event,
//      GstEditorElement * element);
  gint (*event) (GooCanvasItemSimple * item, GdkEvent * event,
      GstEditorElement * element);
};


GType gst_editor_element_get_type (void);
void gst_editor_element_move (GstEditorElement * element,
    gdouble dx, gdouble dy);
void gst_editor_element_cut (GstEditorElement * element);
void gst_editor_element_copy (GstEditorElement * element);
void gst_editor_element_remove (GstEditorElement * element);

void gst_editor_element_stop_child (GstEditorElement * child);

gboolean gst_editor_element_sync_state (GstEditorElement * element);

/*
 * FIXME: This is not used in the GstEditorElement class but only
 * by GtkEditorBin.
 */
void gst_editor_element_realize (GooCanvasItem * citem);

#endif /* __GST_EDITOR_ELEMENT_H__ */

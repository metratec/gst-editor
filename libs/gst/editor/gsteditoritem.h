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


#ifndef __GST_EDITOR_ITEM_H__
#define __GST_EDITOR_ITEM_H__

#include <string.h>

#include <goocanvas.h>
#include <gio/gio.h>
#include <gst/gst.h>
#include <gst/editor/editor.h>
#include <gst/common/gste-serialize.h>

#define GST_TYPE_EDITOR_ITEM (gst_editor_item_get_type())
#define GST_EDITOR_ITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_EDITOR_ITEM, GstEditorItem))
#define GST_EDITOR_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_EDITOR_ITEM, GstEditorItemClass))
#define GST_IS_EDITOR_ITEM(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_EDITOR_ITEM))
#define GST_IS_EDITOR_ITEM_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_EDITOR_ITEM))
#define GST_EDITOR_ITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_EDITOR_ITEM, GstEditorItemClass))


/*
 * This is the editor item class. It is the base class for editor pads and
 * editor elements. It provides a text label and a box with a border,
 * convenience routimes for relating to gstobjects, and a common resizing and
 * repacking system.
 *
 * The box model of the editor item is like this:
 *
 * +-------------------------------------+
 * |                T                    | L = left
 * | - - - - - - - - - - - - - - - - - - | R = right
 * |    |                            |   | T = top
 * |                                     | B = bottom
 * | L  |           C                | R | C = contents
 * |                                     |
 * |    |                            |   |
 * | - - - - - - - - - - - - - - - - - - |
 * |                B                    |
 * +-------------------------------------+
 *
 * Each of these bands has a minimum width and height. If the actual width and
 * height of the entire element cannot accomodate these minimum dimensions, it
 * is resized up.
 *
 * Initially, the item is realized. Child classes should chain up before
 * realizing themselves. The realize method corresponding to the object's type
 * then calls gst_editor_item_resize, which will reset the minimum band sizes
 * and then call the object resize method, which will chain up eventually to an
 * internal item resize routine. The item resize method will then invoke the
 * object's repack method, which will actually place all of the child items.
 *
 * 
 */

typedef struct
{
  gdouble w, h;
} GstEditorItemBand;

struct _GstEditorItem
{
  GooCanvasGroup group;

  /*
   * Every item needs its own popup action group,
   * so we can 'link' the actions to this instance.
   */
  GActionGroup *action_group;
  GtkWidget *menu;

  /* we are a 'view' of this object */
  GstObject *object;

  /* visual stuff */
  GooCanvasItem *border;
  GooCanvasItem *title;

  gulong notify_cb_id;		/* for element property notification */

  gdouble width, height;
  GstEditorItemBand l, r, t, b/*, c*/;

  gchar *title_text;
  gdouble textx, texty;
  GooCanvasAnchorType textanchor;

  gboolean resize;
  gboolean realized;

  /* these apply to the border */
  guint32 fill_color;
  guint32 outline_color;

  GRWLock *globallock;
};

struct _GstEditorItemClass
{
  GooCanvasGroupClass parent_class;

  void (*resize) (GstEditorItem * item);
  void (*repack) (GstEditorItem * item);
  void (*object_changed) (GstEditorItem * item, GstObject * object);
  void (*position_changed) (GstEditorItem * item);

  /* virtual method, does not chain up */
  void (*whats_this) (GstEditorItem * item);

  GMenu *gmenu;
};

/* struct to keep positional info on EditorItem around before it lives */
/* FIXME: maybe this should move out of header so not everyone can poke ? */
typedef struct
{
  gchar *name;
  gdouble x, y;
  gdouble w, h;
} GstEditorItemAttr;

GType gst_editor_item_get_type (void);
void gst_editor_item_resize (GstEditorItem * item);
void gst_editor_item_repack (GstEditorItem * item);
GstEditorItem *gst_editor_item_get (GstObject * object);
void gst_editor_item_move (GstEditorItem * item, gdouble dx, gdouble dy);
void gst_editor_item_disconnect (GstEditorItem * parent,GstEditorItem * child);
void gst_editor_item_hash_remove (GstObject * object);

gchar *gst_editor_item_save (GstEditorItem * item, GsteSerializeFlags flags);
void gst_editor_item_save_with_metadata (GstEditorItem * item, GKeyFile * key_file,
    GsteSerializeFlags flags);

/*
 * FIXME: We should not have to export a realize callback
 * (it's not even used in the GstEditorItem class).
 */
void gst_editor_item_realize (GooCanvasItem * citem);

#endif /* __GST_EDITOR_ITEM_H__ */

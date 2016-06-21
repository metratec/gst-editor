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
#include "config.h"

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/editor/editor.h>
#include <gst/common/gste-debug.h>
#include <gst/editor/gsteditorelement.h>

#include "gst-helper.h"
/* interface methods */
static void canvas_item_interface_init (GooCanvasItemIface * iface);
static gboolean gst_editor_pad_enter_notify_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventCrossing * event);
static gboolean gst_editor_pad_leave_notify_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventCrossing * event);
static gboolean gst_editor_pad_motion_notify_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventMotion * event);
static gboolean gst_editor_pad_button_press_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventButton * event);
static gboolean gst_editor_pad_button_release_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventButton * event);

/* class functions */
static void gst_editor_pad_base_init (GstEditorPadClass * klass);
static void gst_editor_pad_class_init (GstEditorPadClass * klass);
static void gst_editor_pad_init (GstEditorPad * pad);
static gboolean gst_editor_pad_realize_source (GooCanvasItem * citem);
static void gst_editor_pad_resize (GstEditorItem * item);
static void gst_editor_pad_repack (GstEditorItem * item);

static void gst_editor_pad_repack (GstEditorItem * item);
static void gst_editor_pad_object_changed (GstEditorItem * item,
    GstObject * object);

/* callbacks on GstPad */

static void on_pad_linked (GstPad  *pad,GstPad  *peer,GstEditorItem * item);
static void on_pad_parent_notify (GstObject * object, GParamSpec * pspec,
    GstEditorItem * item);

/* utility functions */

static void gst_editor_pad_link_drag (GstEditorPad * pad,
    gdouble wx, gdouble wy);
static void gst_editor_pad_link_start (GstEditorPad * pad);

static void on_pad_status (GSimpleAction * action,
    GVariant * parameter, gpointer user_data);
static void on_derequest_pad (GSimpleAction * action,
    GVariant * parameter, gpointer user_data);
static void on_ghost (GSimpleAction * action,
    GVariant * parameter, gpointer user_data);
static void on_remove_ghost_pad (GSimpleAction * action,
    GVariant * parameter, gpointer user_data);
static void on_request_pad (GSimpleAction * action,
    GVariant * parameter, gpointer user_data);
static void on_frobate (GSimpleAction * action,
    GVariant * parameter, gpointer user_data);

struct help
{

  GMutex *proxy_lock;
  GstPad *target;
  GstPad *internal;
};


enum
{
  ARG_0,
};

enum
{
  LAST_SIGNAL
};

static GstEditorItemClass *parent_class;

static const GActionEntry action_entries[] = {
      {"status", on_pad_status, NULL, NULL, NULL},
      {"ghost", on_ghost, NULL, NULL, NULL},
      {"remove", on_remove_ghost_pad, NULL, NULL, NULL},
      {"release", on_derequest_pad, NULL, NULL, NULL},
      {"request", on_request_pad, NULL, NULL, NULL},
      {"frobate", on_frobate, NULL, NULL, NULL}
};

static const char *always_pad_ui_description =
    "<interface>"
      "<menu id='itemMenu'>"
        "<section id='padSection'>"
          "<item>"
            "<attribute name='label' translatable='yes'>Pad status...</attribute>"
            "<attribute name='tooltip' translatable='yes'>Query pad caps, formats, etc</attribute>"
            "<attribute name='icon'>document-properties</attribute>"
            "<attribute name='action'>local.status</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Ghost to parent bin...</attribute>"
            "<attribute name='tooltip' translatable='yes'>Ghost this pad to the parent bin</attribute>"
            "<attribute name='icon'>go-jump</attribute>"
            "<attribute name='action'>local.ghost</attribute>"
          "</item>"
        "</section>"
      "</menu>"
    "</interface>";

static const char *ghost_pad_ui_description =
    "<interface>"
      "<menu id='itemMenu'>"
        "<section id='padSection'>"
          "<item>"
            "<attribute name='label' translatable='yes'>Pad status...</attribute>"
            "<attribute name='tooltip' translatable='yes'>Query pad caps, formats, etc</attribute>"
            "<attribute name='icon'>document-properties</attribute>"
            "<attribute name='action'>local.status</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Remove ghost pad</attribute>"
            "<attribute name='tooltip' translatable='yes'>De-request this previously-requested pad</attribute>"
            "<attribute name='action'>local.remove</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Ghost to parent bin...</attribute>"
            "<attribute name='tooltip' translatable='yes'>Ghost this pad to the parent bin</attribute>"
            "<attribute name='icon'>go-jump</attribute>"
            "<attribute name='action'>local.ghost</attribute>"
          "</item>"
        "</section>"
      "</menu>"
    "</interface>";

static const char *requested_pad_ui_description =
    "<interface>"
      "<menu id='itemMenu'>"
        "<section id='padSection'>"
          "<item>"
            "<attribute name='label' translatable='yes'>Pad status...</attribute>"
            "<attribute name='tooltip' translatable='yes'>Query pad caps, formats, etc</attribute>"
            "<attribute name='icon'>document-properties</attribute>"
            "<attribute name='action'>local.status</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Release request pad</attribute>"
            "<attribute name='tooltip' translatable='yes'>Release this previously-requested pad</attribute>"
            "<attribute name='action'>local.release</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Ghost to parent bin...</attribute>"
            "<attribute name='tooltip' translatable='yes'>Ghost this pad to the parent bin</attribute>"
            "<attribute name='icon'>go-jump</attribute>"
            "<attribute name='action'>local.ghost</attribute>"
          "</item>"
        "</section>"
      "</menu>"
    "</interface>";

static const char *request_pad_ui_description =
    "<interface>"
      "<menu id='itemMenu'>"
        "<section id='padSection'>"
          "<item>"
            "<attribute name='label' translatable='yes'>Request pad by name...</attribute>"
            "<attribute name='tooltip' translatable='yes'>Request a pad from this template by name</attribute>"
            "<attribute name='action'>local.request</attribute>"
          "</item>"
        "</section>"
      "</menu>"
    "</interface>";

static const char *sometimes_pad_ui_description =
    "<interface>"
      "<menu id='itemMenu'>"
        "<section id='padSection'>"
          "<item>"
            "<attribute name='label' translatable='yes'>Frobate...</attribute>"
            "<attribute name='tooltip' translatable='yes'>Frobate this pad into a cromulate mass of goo</attribute>"
            "<attribute name='icon'>system-run</attribute>"
            "<attribute name='action'>local.frobate</attribute>"
          "</item>"
        "</section>"
      "</menu>"
    "</interface>";

GType
gst_editor_pad_get_type (void)
{
  static GType pad_type = 0;

  if (pad_type == 0) {
    static const GTypeInfo pad_info = {
      sizeof (GstEditorPadClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) NULL,
      NULL,
      NULL,
      sizeof (GstEditorPad),
      0,
      (GInstanceInitFunc) NULL,
    };

    static const GInterfaceInfo iitem_info = {
      (GInterfaceInitFunc) canvas_item_interface_init,  /* interface_init */
      NULL,                     /* interface_finalize */
      NULL                      /* interface_data */
    };

    pad_type =
        g_type_register_static (gst_editor_item_get_type (), "GstEditorPad",
        &pad_info, 0);

    g_type_add_interface_static (pad_type, GOO_TYPE_CANVAS_ITEM, &iitem_info);
  }
  return pad_type;
}

GType
gst_editor_pad_always_get_type (void)
{
  static GType pad_always_type = 0;

  if (!pad_always_type) {
    static const GTypeInfo pad_always_info = {
      sizeof (GstEditorPadAlwaysClass),
      (GBaseInitFunc) gst_editor_pad_base_init,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gst_editor_pad_class_init,
      NULL,
      NULL,
      sizeof (GstEditorPadAlways),
      0,
      (GInstanceInitFunc) gst_editor_pad_init,
    };

    pad_always_type =
        g_type_register_static (gst_editor_pad_get_type (),
        "GstEditorPadAlways", &pad_always_info, 0);
  }
  return pad_always_type;
}

GType
gst_editor_pad_sometimes_get_type (void)
{
  static GType pad_sometimes_type = 0;

  if (!pad_sometimes_type) {
    static const GTypeInfo pad_sometimes_info = {
      sizeof (GstEditorPadSometimesClass),
      (GBaseInitFunc) gst_editor_pad_base_init,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gst_editor_pad_class_init,
      NULL,
      NULL,
      sizeof (GstEditorPadSometimes),
      0,
      (GInstanceInitFunc) gst_editor_pad_init,
    };

    pad_sometimes_type =
        g_type_register_static (gst_editor_pad_get_type (),
        "GstEditorPadSometimes", &pad_sometimes_info, 0);
  }
  return pad_sometimes_type;
}

GType
gst_editor_pad_request_get_type (void)
{
  static GType pad_request_type = 0;

  if (!pad_request_type) {
    static const GTypeInfo pad_request_info = {
      sizeof (GstEditorPadRequestClass),
      (GBaseInitFunc) gst_editor_pad_base_init,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gst_editor_pad_class_init,
      NULL,
      NULL,
      sizeof (GstEditorPadRequest),
      0,
      (GInstanceInitFunc) gst_editor_pad_init,
    };

    pad_request_type =
        g_type_register_static (gst_editor_pad_get_type (),
        "GstEditorPadRequest", &pad_request_info, 0);
  }
  return pad_request_type;
}

GType
gst_editor_pad_requested_get_type (void)
{
  static GType pad_requested_type = 0;

  if (!pad_requested_type) {
    static const GTypeInfo pad_requested_info = {
      sizeof (GstEditorPadRequestedClass),
      (GBaseInitFunc) gst_editor_pad_base_init,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gst_editor_pad_class_init,
      NULL,
      NULL,
      sizeof (GstEditorPadRequested),
      0,
      (GInstanceInitFunc) gst_editor_pad_init,
    };

    pad_requested_type =
        g_type_register_static (gst_editor_pad_get_type (),
        "GstEditorPadRequested", &pad_requested_info, 0);
  }
  return pad_requested_type;
}

GType
gst_editor_pad_ghost_get_type (void)
{
  static GType pad_ghost_type = 0;

  if (!pad_ghost_type) {
    static const GTypeInfo pad_ghost_info = {
      sizeof (GstEditorPadGhostClass),
      (GBaseInitFunc) gst_editor_pad_base_init,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gst_editor_pad_class_init,
      NULL,
      NULL,
      sizeof (GstEditorPadGhost),
      0,
      (GInstanceInitFunc) gst_editor_pad_init,
    };

    pad_ghost_type =
        g_type_register_static (gst_editor_pad_get_type (), "GstEditorPadGhost",
        &pad_ghost_info, 0);
  }
  return pad_ghost_type;
}

static void
gst_editor_pad_base_init (GstEditorPadClass * klass)
{
  GstEditorItemClass *item_class = GST_EDITOR_ITEM_CLASS (klass);
  GtkBuilder *ui_builder = NULL;

  if (G_TYPE_FROM_CLASS (klass) == GST_TYPE_EDITOR_PAD_ALWAYS) {
    ui_builder = gtk_builder_new_from_string (always_pad_ui_description, -1);
//    item_class->whats_this = gst_editor_pad_always_whats_this;
  } else if (G_TYPE_FROM_CLASS (klass) == GST_TYPE_EDITOR_PAD_SOMETIMES) {
    ui_builder = gtk_builder_new_from_string (sometimes_pad_ui_description, -1);
//    item_class->whats_this = gst_editor_pad_sometimes_whats_this;
  } else if (G_TYPE_FROM_CLASS (klass) == GST_TYPE_EDITOR_PAD_REQUEST) {
    ui_builder = gtk_builder_new_from_string (request_pad_ui_description, -1);
//    item_class->whats_this = gst_editor_pad_request_whats_this;
  } else if (G_TYPE_FROM_CLASS (klass) == GST_TYPE_EDITOR_PAD_REQUESTED) {
    ui_builder = gtk_builder_new_from_string (requested_pad_ui_description, -1);
//    item_class->whats_this = gst_editor_pad_requested_whats_this;
  } else if (G_TYPE_FROM_CLASS (klass) == GST_TYPE_EDITOR_PAD_GHOST) {
    ui_builder = gtk_builder_new_from_string (ghost_pad_ui_description, -1);
//    item_class->whats_this = gst_editor_pad_ghost_whats_this;
  }

  g_menu_prepend_section (item_class->gmenu, NULL,
      G_MENU_MODEL (gtk_builder_get_object (ui_builder, "padSection")));

  g_object_unref (ui_builder);
}

static void
gst_editor_pad_class_init (GstEditorPadClass * klass)
{
  GstEditorItemClass *item_class = GST_EDITOR_ITEM_CLASS (klass);

  parent_class = g_type_class_ref (gst_editor_item_get_type ());

#if 0
  citem_class->realize = gst_editor_pad_realize;
#endif
  item_class->resize = gst_editor_pad_resize;
  item_class->repack = gst_editor_pad_repack;
  item_class->object_changed = gst_editor_pad_object_changed;
}

static void
canvas_item_interface_init (GooCanvasItemIface * iface)
{
  iface->enter_notify_event = gst_editor_pad_enter_notify_event;
  iface->leave_notify_event = gst_editor_pad_leave_notify_event;
  iface->motion_notify_event = gst_editor_pad_motion_notify_event;
  iface->button_press_event = gst_editor_pad_button_press_event;
  iface->button_release_event = gst_editor_pad_button_release_event;
}

static void
gst_editor_pad_init (GstEditorPad * pad)
{
  GType type;
  GstEditorItem *item = GST_EDITOR_ITEM (pad);

  g_action_map_add_action_entries (G_ACTION_MAP (item->action_group),
      action_entries, G_N_ELEMENTS (action_entries), pad);

  type = G_OBJECT_TYPE (pad);

  if (type == GST_TYPE_EDITOR_PAD_ALWAYS) {
    pad->presence = GST_PAD_ALWAYS;
    item->fill_color = 0xffccccff;      // 0xffcccc00;
  } else if (type == GST_TYPE_EDITOR_PAD_SOMETIMES) {
    pad->istemplate = TRUE;
    pad->presence = GST_PAD_SOMETIMES;
    item->fill_color = 0xccffccff;      //0xccffcc00;
  } else if (type == GST_TYPE_EDITOR_PAD_REQUEST) {
    pad->istemplate = TRUE;
    pad->presence = GST_PAD_REQUEST;
    item->fill_color = 0xccccffff;      //0xccccff00;
  } else if (type == GST_TYPE_EDITOR_PAD_REQUESTED) {
    pad->presence = GST_PAD_ALWAYS;
    item->fill_color = 0xffccccff;      //0xffcccc00;
  } else if (type == GST_TYPE_EDITOR_PAD_GHOST) {
    pad->presence = GST_PAD_ALWAYS;
    item->fill_color = 0xccccccff;      //0xcccccc00;
  } else {
    g_assert_not_reached ();
  }

  item->outline_color = 0x000000ff;



  GST_CAT_DEBUG (gste_debug_cat, "new pad of type %s (%p)\n",
      g_type_name (G_OBJECT_TYPE (pad)), pad);
}

/*static*/ void
gst_editor_pad_realize (GooCanvasItem * citem)
{
  GstEditorItem *item;
  GstEditorPad *pad;

  item = GST_EDITOR_ITEM (citem);
  pad = GST_EDITOR_PAD (citem);

  /* keep in mind item->object can be a Pad or a PadTemplate; they are the same
   * in the mind of a user just wanting to connect two elements. this function
   * should work for both kinds of object. */

  g_return_if_fail (item->object != NULL);
  if (!item->realized) {
    gst_editor_item_realize (citem);
  } else {  // check if links are still valid
    gboolean fail = FALSE;
    if (pad->ghostlink) {
      GstPad *suggestedpeer;
      GstEditorItem *suggestedpeerpad;
      if (GST_IS_EDITOR_LINK (pad->ghostlink) &&
          (pad->ghostlink->srcpad == item ||
           pad->ghostlink->sinkpad == item)) {
        suggestedpeerpad =
            pad->ghostlink->srcpad == item ? pad->ghostlink->sinkpad
                                           : pad->ghostlink->srcpad;
        suggestedpeer = gst_ghost_pad_get_target (GST_GHOST_PAD (item->object));
        if (!suggestedpeer || !suggestedpeerpad ||
            !GST_IS_EDITOR_ITEM (suggestedpeerpad) ||
            GST_PAD (suggestedpeerpad->object) != suggestedpeer) {
          g_print (
              "gst_editor_pad_realize failed because ghost target does not "
              "match, suggestedpeerpad is %p\n",
              suggestedpeerpad);
          fail = TRUE;
          if (GOO_IS_CANVAS_ITEM (pad->ghostlink))
            goo_canvas_item_remove (GOO_CANVAS_ITEM (pad->ghostlink));
          pad->ghostlink = NULL;
          g_idle_add ((GSourceFunc)gst_editor_pad_realize_source, pad);
        }
      } else {
        g_print (
            "gst_editor_pad_realize failed because ghostlink is not "
            "editorlink\n");
        fail = TRUE;
      }
    }
    if (pad->link) {
      GstPad * suggestedpeer;
      GstEditorItem * suggestedpeerpad;
      if (GST_IS_EDITOR_LINK (pad->link) &&
          (pad->link->srcpad == item || pad->link->sinkpad == item)) {
        suggestedpeerpad = pad->link->srcpad == item ? pad->link->sinkpad
                                                     : pad->link->srcpad;
        suggestedpeer = gst_pad_get_peer (GST_PAD (item->object));
        if (!suggestedpeer || !suggestedpeerpad ||
            !GST_IS_EDITOR_ITEM (suggestedpeerpad) ||
            GST_PAD (suggestedpeerpad->object) != suggestedpeer) {
          g_print (
              "gst_editor_pad_realize failed because target does not match, "
              "suggestedpeerpad is %p\n",
              suggestedpeerpad);
          fail = TRUE;
        }
      } else {
        g_print (
            "gst_editor_pad_realize failed because link is not editorlink\n");
        fail = TRUE;
      }
    }
    if (fail) {  // wait until someone else cleaned it up...
      // g_idle_add ((GSourceFunc)gst_editor_pad_realize_source,pad);
      // sleep(5);
      return;
    }
  }
  if (pad->istemplate) {
    pad->issrc =
        (GST_PAD_TEMPLATE_DIRECTION (GST_PAD_TEMPLATE (item->object)) ==
        GST_PAD_SRC);
  } else {
    pad->issrc = (GST_PAD_DIRECTION (GST_PAD (item->object)) == GST_PAD_SRC);
  }

  if (G_OBJECT_TYPE (pad) == gst_editor_pad_ghost_get_type ())
    pad->isghost = TRUE;
  if (!item->realized){
    if (pad->issrc || pad->isghost)
          pad->srcbox =
          goo_canvas_rect_new (GOO_CANVAS_ITEM (citem),
            0., 0., 0., 0.,
        	"line-width", 1.,
            "fill_color", "white", "stroke_color", "black",
            "antialias", CAIRO_ANTIALIAS_NONE, NULL);
  	if (!pad->issrc || pad->isghost)
    	pad->sinkbox =
        goo_canvas_rect_new (GOO_CANVAS_ITEM (citem),
            0., 0., 0., 0.,
        	"line-width", 1.,
            "fill_color", "white", "stroke_color", "black",
            "antialias", CAIRO_ANTIALIAS_NONE, NULL);
   }
  if (!pad->istemplate) {
    GstPad *_pad, *_peer;

    _pad = GST_PAD (item->object);
    _peer = GST_PAD_PEER (_pad);

    if (_peer) {
      /* we have a linked pad */
      GstEditorPad *peer =
          (GstEditorPad *) gst_editor_item_get ((GstObject *) _peer);

      if (peer) {
        if ((!pad->link)&&(!peer->link)){
          GooCanvasItem * link;

          g_message ("linking GUI for %s:%s and %s:%s", GST_DEBUG_PAD_NAME (_pad),
            GST_DEBUG_PAD_NAME (_peer));
          link = goo_canvas_item_new (GOO_CANVAS_ITEM (citem),
            gst_editor_link_get_type (), NULL);
          gst_editor_link_realize (link);
          if (pad->issrc)
            g_object_set (G_OBJECT (link), "src-pad", pad, "sink-pad", peer,
              NULL);
          else
            g_object_set (G_OBJECT (link), "sink-pad", pad, "src-pad", peer,
              NULL);

          gst_editor_link_link (GST_EDITOR_LINK (link));
	}
	else g_print("Pad or peer linked already");
      }
      else g_print("Warning: Peer-canvas is NULL");
    }

    /*if (GST_IS_REAL_PAD (_pad) && GST_REAL_PAD (_pad)->ghostpads) {
       GstEditorPad *peer;
       GList *l;

       for (l = GST_REAL_PAD (_pad)->ghostpads; l; l = l->next) {
       GnomeCanvasItem *link;

       _peer = GST_PAD (l->data);
       peer = (GstEditorPad *) gst_editor_item_get ((GstObject *) _peer);

       g_return_if_fail (peer != NULL);

       g_message ("linking ghost pad for %s:%s and %s:%s",
       GST_DEBUG_PAD_NAME (_pad), GST_DEBUG_PAD_NAME (_peer));
       link =
       gnome_canvas_item_new (GNOME_CANVAS_GROUP (citem),
       gst_editor_link_get_type (), NULL);
       gnome_canvas_item_set (link, "ghost", TRUE, NULL);
       if (pad->issrc)
       gnome_canvas_item_set (link, "src-pad", pad, "sink-pad", peer, NULL);
       else
       gnome_canvas_item_set (link, "sink-pad", pad, "src-pad", peer, NULL);

       gst_editor_link_link (GST_EDITOR_LINK (link));
       }
       } */
  }
  else g_print("pad is template, not linking");

  if (pad->isghost && !pad->ghostlink) {
    /*GnomeCanvasItem *link;
       GstPad *_pad, *_peer;
       GstEditorPad *peer;

       _pad = GST_PAD (item->object);
       _peer = (GstPad *) GST_PAD_REALIZE (_pad);

       peer = (GstEditorPad *) gst_editor_item_get ((GstObject *) _peer);

       g_return_if_fail (peer != NULL);

       g_message ("link ghost pad for %s:%s and %s:%s", GST_DEBUG_PAD_NAME (_pad),
       GST_DEBUG_PAD_NAME (_peer));
       link = gnome_canvas_item_new (GNOME_CANVAS_GROUP (citem),
       gst_editor_link_get_type (), NULL);
       gnome_canvas_item_set (link, "ghost", TRUE, NULL);
       if (!peer->issrc)
       gnome_canvas_item_set (link, "src-pad", pad, "sink-pad", peer, NULL);
       else
       gnome_canvas_item_set (link, "sink-pad", pad, "src-pad", peer, NULL);

       gst_editor_link_link (GST_EDITOR_LINK (link)); */ 
    GooCanvasItem * link;
    GstPad * _pad, *_peer;
    GstEditorPad * peer = NULL;
    _pad = GST_PAD (item->object);
    if (GST_IS_GHOST_PAD (_pad)) {
      _peer = gst_ghost_pad_get_target (GST_GHOST_PAD (_pad));
      peer = (GstEditorPad *) gst_editor_item_get ((GstObject *) _peer);
    }
    g_return_if_fail (peer != NULL);
    g_message ("link ghost pad for %s:%s and %s:%s, pointers: pad:%p peer:%p",
        GST_DEBUG_PAD_NAME (_pad), GST_DEBUG_PAD_NAME (_peer),_pad,_peer);
    link =
        goo_canvas_item_new (GOO_CANVAS_ITEM (citem),
        gst_editor_link_get_type (), NULL);
    g_object_set (G_OBJECT (link), "ghost", TRUE, NULL);
    if (!peer->issrc)
      g_object_set (G_OBJECT (link), "src-pad", pad, "sink-pad", peer, NULL);
    else
      g_object_set (G_OBJECT (link), "sink-pad", pad, "src-pad", peer, NULL);
    gst_editor_link_link (GST_EDITOR_LINK (link));
  }

  item->realized = TRUE;

  /* we know nothing will be derived from us */
  gst_editor_item_resize (item);
}

static gboolean
gst_editor_pad_realize_source (GooCanvasItem * citem)
{
  if (!GOO_IS_CANVAS_ITEM (citem)) {
    g_print ("Not a valid canvas item anymore, stopping\n");
    return FALSE;
  }
  if (!GST_IS_EDITOR_PAD (citem)) {
    g_print ("Not a valid EditorPad anymore, stopping\n");
    return FALSE;
  }
  g_rw_lock_writer_lock (GST_EDITOR_ITEM (citem)->globallock);
  gst_editor_pad_realize (citem);
  g_rw_lock_writer_unlock (GST_EDITOR_ITEM (citem)->globallock);
  return FALSE;
}

static void
gst_editor_pad_resize (GstEditorItem * item)
{
  GstEditorPad *pad = GST_EDITOR_PAD (item);

  /* the link box */
  item->t.w += 4.0;
  item->t.h = MAX (item->t.h, 8.0);

  if (pad->isghost)
    item->t.w += 4.0;

  if (!pad->issrc || pad->isghost)
    item->textx = 5.0;

  GST_EDITOR_ITEM_CLASS (parent_class)->resize (item);
}

static void
gst_editor_pad_repack (GstEditorItem * item)
{
  GstEditorPad *pad = GST_EDITOR_PAD (item);

  if (!item->realized)
    return;

  if (pad->srcbox) {
    g_object_set (pad->srcbox, "x", item->width - 2.0, "y", /*item->height - */
        2.0, 
        "width", 2.0, "height", item->height - 4.0, NULL);
  }

  if (pad->sinkbox) {
    g_object_set (pad->sinkbox, "x", 0.0, "y", /*item->height - */ 2.0, 
        "width", 2.0, "height", item->height - 4.0, NULL);
  }

  if (GST_EDITOR_ITEM_CLASS (parent_class)->repack)
    (GST_EDITOR_ITEM_CLASS (parent_class)->repack) (item);
}

static void
gst_editor_pad_object_changed (GstEditorItem * item, GstObject * object)
{
  GstObject *old = item->object;

  if (old){
    g_signal_handlers_disconnect_by_func (old, G_CALLBACK (on_pad_parent_notify),
        item);
    g_signal_handlers_disconnect_by_func (old, G_CALLBACK (on_pad_linked),
        item);
  }
  if (object){
    g_print("GST_editor_Pad: Connecting signals!!");
    g_signal_connect (object, "notify::parent", G_CALLBACK (on_pad_parent_notify),
        item);
    g_signal_connect (object, "linked", G_CALLBACK (on_pad_linked),
        item);
  }
  parent_class->object_changed (item, object);
}

static gboolean 
gst_editor_pad_enter_notify_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventCrossing * event) 
{
  GstEditorItem * item = GST_EDITOR_ITEM (citem);
  g_object_set (GOO_CANVAS_ITEM (item->border), "fill_color_rgba",
      0xBBDDBBFF /*0xBBDDBB00 */ , NULL);
  
//   if (GOO_CANVAS_ITEM_GET_IFACE (goo_canvas_item_get_parent(citem))->enter_notify_event)
//      return GOO_CANVAS_ITEM_GET_IFACE (goo_canvas_item_get_parent(citem))->enter_notify_event (citem, target, event);
  return FALSE;
}

static gboolean 
gst_editor_pad_leave_notify_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventCrossing * event) 
{
  GstEditorPad * pad;
  GstEditorItem * item;
  item = GST_EDITOR_ITEM (citem);
  pad = GST_EDITOR_PAD (citem);
  g_object_set (GOO_CANVAS_ITEM (item->border), "fill_color_rgba",
      item->fill_color, NULL);
  if (pad->unlinking) {
    GstEditorPad * otherpad;
    otherpad = 
        (GstEditorPad *) ((pad == 
            (GstEditorPad *) pad->link->srcpad) ? pad->link->sinkpad : pad->
        link->srcpad);
    gst_editor_link_unlink (pad->link);
    gst_editor_pad_link_start (otherpad);
  }
  pad->unlinking = FALSE;
  
//   if (GOO_CANVAS_ITEM_GET_IFACE (goo_canvas_item_get_parent(citem))->leave_notify_event)
//      return GOO_CANVAS_ITEM_GET_IFACE (goo_canvas_item_get_parent(citem))->leave_notify_event (citem, target, event);
  return FALSE;
}

static gboolean 
gst_editor_pad_motion_notify_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventMotion * event) 
{
  GstEditorPad * pad = GST_EDITOR_PAD (citem);
  if (pad->linking) {
    gdouble x, y;
    x = event->x_root;
    y = event->y_root;
    gst_editor_pad_link_drag (pad, x, y);
    return TRUE;
  }
  
/*
  if (GNOME_CANVAS_ITEM_CLASS (parent_class)->event)
    return GNOME_CANVAS_ITEM_CLASS (parent_class)->event (citem, event);

  if (GOO_CANVAS_ITEM_GET_IFACE (GST_EDITOR_ITEM(citem))->motion_notify_event)
    return GOO_CANVAS_ITEM_GET_IFACE (GST_EDITOR_ITEM(citem))->motion_notify_event (citem, target, event);
*/ 
  return FALSE;
}

static gboolean 
gst_editor_pad_button_press_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventButton * event) 
{
  GooCanvasItemIface * iface;
  GstEditorPad * pad = GST_EDITOR_PAD (citem);
  if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
    if (!pad->link)
      gst_editor_pad_link_start (pad);
    else
      pad->unlinking = TRUE;
    return TRUE;
  }
  
//   if (GOO_CANVAS_ITEM_GET_IFACE (goo_canvas_item_get_parent(citem))->button_press_event)
//      return GOO_CANVAS_ITEM_GET_IFACE (goo_canvas_item_get_parent(citem))->button_press_event (citem, target, event);
  iface =
      (GooCanvasItemIface *) g_type_interface_peek (parent_class,
      GOO_TYPE_CANVAS_ITEM);
  if (iface->button_press_event)
    return iface->button_press_event (citem, target, event);

  return FALSE;
}

static gboolean 
gst_editor_pad_button_release_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventButton * event) 
{
  GstEditorLink * link;
  GstEditorPad * pad = GST_EDITOR_PAD (citem);
  if (event->button == 1) {
    pad->unlinking = FALSE;
    if (pad->linking) {
      g_assert (pad->link != NULL);
      
          //       gnome_canvas_item_ungrab (citem, event->button.time);
      goo_canvas_pointer_ungrab (goo_canvas_item_get_canvas (citem), citem,
          event->time);
      link = pad->link;
      if (!gst_editor_link_link (link)) {
        
            //newly added link-destroy function, kicks link from Pad-Canvas
	    gst_editor_link_destroy(link);

      }
      pad->linking = FALSE;
      return TRUE;
    }
  }
  
//   if (GOO_CANVAS_ITEM_GET_IFACE (goo_canvas_item_get_parent(citem))->button_release_event)
//      return GOO_CANVAS_ITEM_GET_IFACE (goo_canvas_item_get_parent(citem))->button_release_event (citem, target, event);
      return FALSE;
}

static void
on_pad_linked (GstPad * pad, GstPad * peer, GstEditorItem * item)
{
  g_rw_lock_writer_lock (GST_EDITOR_ITEM (item)->globallock);
  g_print (
      "linking! Padpointer: %p, Peer %p Linkpointer %p, Ghostlinkpointer %p\n",
      pad, peer, GST_EDITOR_PAD (item)->link, GST_EDITOR_PAD (item)->ghostlink);
  if ((!GST_IS_PAD (pad)) || (!GST_IS_PAD (peer))) {
    g_print ("Warning, pad or peer invalid");
    return;
  }
  if (!gst_pad_get_parent_element (pad))
    g_print ("pad %s has no parent\n", gst_pad_get_name (pad));

  else if (!gst_pad_get_parent_element (peer))
    g_print ("peer %s has no parent, we are %s:%s\n", gst_pad_get_name (peer),
        GST_ELEMENT_NAME (gst_pad_get_parent_element (pad)),
        gst_pad_get_name (pad));
  else
    g_print ("Element %s, pad %s linked to Element %s, pad %s\n",
        GST_ELEMENT_NAME (gst_pad_get_parent_element (pad)),
        gst_pad_get_name (pad),
        GST_ELEMENT_NAME (gst_pad_get_parent_element (peer)),
        gst_pad_get_name (peer));
  /* if (GST_EDITOR_PAD (item)->ghostlink){
      gst_editor_link_unlink (GST_EDITOR_PAD (item)->link);
      goo_canvas_item_remove(GOO_CANVAS_ITEM(item));
     } */
  if (GST_IS_GHOST_PAD (pad))
    g_print ("Pad is ghost pad\n");
  if (GST_IS_GHOST_PAD (peer))
    g_print ("Peer is ghost pad\n");
  if (GST_PAD_PEER (pad) != peer)
    g_print ("Warning, linked signal received also peer is not peer of pad\n");
  if (!gst_pad_get_parent_element (peer)) {
    // find bin to update pads...because the callback did not give us sufficient
    // information to do it on our own
    GstObject * _bin;
    GstEditorItem * bin = NULL;
    _bin = gst_element_get_parent (gst_pad_get_parent_element (pad));
    if (_bin)
      bin = gst_editor_item_get (_bin);
    if (bin) {
      g_print ("Warning, trying to add pads with links!!\n");
      // gst_editor_element_add_pads(bin);
      // find the pad that is connected to us...
      GstIterator * pads;
      GValue item = G_VALUE_INIT;
      GstPad * _refreshpad;
      gboolean done = FALSE;
      pads = gst_element_iterate_pads (GST_ELEMENT (bin->object));
      while (!done) {
        switch (gst_iterator_next (pads, &item)) {
          case GST_ITERATOR_OK:
            _refreshpad = GST_PAD (g_value_get_object (&item));
            if (GST_IS_GHOST_PAD (_refreshpad)) {
              g_print ("Ghoastpad found that could be ours: %p\n", _refreshpad);
              GstEditorItem *refreshpad = gst_editor_item_get (GST_OBJECT (_refreshpad));
              if (!refreshpad)
                g_print ("Ghoastpad found has no Object, critical!!!\n");
              g_print (
                  "Ghoastpad Object found Linkpointer %p, Ghostlinkpointer "
                  "%p!!!\n",
                  GST_EDITOR_PAD (refreshpad)->link,
                  GST_EDITOR_PAD (refreshpad)->ghostlink);
              // if (gst_pad_is_blocked(_refreshpad)) g_print("Oh, it is
              // blocked!!\n");
              // GstPad* target;
              // target=(GstPad*)((*((void*)((GST_GHOST_PAD(_refreshpad))->priv)))+sizeof(void*));//Deadlock
              // if done properly since already locked
              // target=((GstPad*)(GST_GHOST_PAD(_refreshpad)->priv+sizeof(void*)));//Deadlock
              // if done properly since already locked
              // target=((struct
              // help*)(GST_GHOST_PAD(_refreshpad)->priv))->target;
              // target=(GstPad*)((void*)(GST_GHOST_PAD(_refreshpad)->priv)+sizeof(GstPad*));

              // g_idle_add()...update link information
              g_idle_add (
                  (GSourceFunc)gst_editor_pad_realize_source, refreshpad);
              // if (target==pad) g_print("Oh, that am I!!\n");
              // else g_print("Oh, that am I not: %p!!\n",target);
              // gst_editor_pad_realize ((GooCanvasItem*)refreshpad);
            }
            gst_object_unref (_refreshpad);
            g_value_reset (&item);
            break;
          case GST_ITERATOR_RESYNC:
            g_print (
                "GST_ITERATOR_RESYNC on Pad linked signal, handling of this "
                "not implemented\n");
            break;
          case GST_ITERATOR_ERROR:
            g_print (
                "GST_ITERATOR_ERROR on Pad linked signal, handling of this not "
                "implemented\n");
          case GST_ITERATOR_DONE:
          default:
            done = TRUE;
            break;
        }
      }
      g_value_unset (&item);
      gst_iterator_free (pads);
    }
  }
  gst_editor_pad_realize (GOO_CANVAS_ITEM (item));
  g_rw_lock_writer_unlock (GST_EDITOR_ITEM (item)->globallock);
}

static void
on_pad_parent_notify (GstObject * object, GParamSpec * pspec,
    GstEditorItem * item)
{
  if (GST_OBJECT_PARENT (object) != NULL)
    return;

  g_rw_lock_writer_lock (GST_EDITOR_ITEM(item)->globallock);
  g_object_set (item, "object", NULL, NULL);

  if (GST_EDITOR_PAD (item)->link)
    gst_editor_link_unlink (GST_EDITOR_PAD (item)->link);
  g_rw_lock_writer_unlock (GST_EDITOR_ITEM(item)->globallock);
  goo_canvas_item_remove(GOO_CANVAS_ITEM(item));
  //goo_canvas_item_simple_hide (GOO_CANVAS_ITEM_SIMPLE (item));

  /* we are removed from the element's pad list with the pad_removed signal */

}

static void
gst_editor_pad_link_start (GstEditorPad * pad)
{
  GdkCursor *cursor;
  GooCanvasItem *link;

  g_return_if_fail (GST_IS_EDITOR_PAD (pad));
  g_return_if_fail (pad->link == NULL);

  link = goo_canvas_item_new (GOO_CANVAS_ITEM (pad),
      gst_editor_link_get_type (), pad->issrc ? "src-pad" : "sink-pad", pad,
      NULL);
  gst_editor_link_realize (link);
  cursor = gdk_cursor_new (GDK_HAND2);
  goo_canvas_pointer_grab (goo_canvas_item_get_canvas (GOO_CANVAS_ITEM (pad)),
      GOO_CANVAS_ITEM (pad),
      GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK, cursor,
      GDK_CURRENT_TIME);
  g_object_unref (cursor);

  pad->linking = TRUE;
}

static void
gst_editor_pad_link_drag (GstEditorPad * pad, gdouble wx, gdouble wy)
{
  GstEditorItem *item;

  GooCanvasItem * underitem, *under = NULL;
  GstEditorPad *destpad = NULL;

  item = GST_EDITOR_ITEM (pad);

  /* if we're on top of an interesting pad */

  if ((underitem =
          goo_canvas_get_item_at (goo_canvas_item_get_canvas (GOO_CANVAS_ITEM
                  (item)), wx, wy, FALSE)))
    under = GST_EDITOR_GET_OBJECT (underitem);

  if (under && GST_IS_EDITOR_PAD (under))
    destpad = GST_EDITOR_PAD (under);

  if (destpad && destpad != pad &&
      (!destpad->link || destpad->link == pad->link) &&
      destpad->issrc != pad->issrc) {
    g_object_set (GOO_CANVAS_ITEM (pad->link),
        pad->issrc ? "sink-pad" : "src-pad", destpad, NULL);
  } else {
    if (pad->issrc ? pad->link->sinkpad : pad->link->srcpad)
      g_object_set (GOO_CANVAS_ITEM (pad->link),
          pad->issrc ? "sink-pad" : "src-pad", NULL, NULL);
      g_object_set (GOO_CANVAS_ITEM (pad->link), "x", wx, "y", wy, NULL);
  }
}

static void
on_pad_status (GSimpleAction * action,
    GVariant * parameter, gpointer user_data)
{
  //GstEditorPadAlways *pad = GST_EDITOR_PAD_ALWAYS (user_data);

  g_print ("pad status\n");
}

static void
on_derequest_pad (GSimpleAction * action,
    GVariant * parameter, gpointer user_data)
{
  GstEditorPadRequested *pad = GST_EDITOR_PAD_REQUESTED (user_data);
  GstPad *rpad = NULL;
  GstElement *element = NULL;

  g_print ("derequest pad\n");

  rpad = GST_PAD (GST_EDITOR_ITEM (pad)->object);
  element = GST_ELEMENT (GST_OBJECT_PARENT (rpad));

  if (!GST_ELEMENT_CLASS (G_OBJECT_GET_CLASS (element))->release_pad)
    g_warning ("Elements of type %s have not implemented release_request_pad",
        g_type_name (G_OBJECT_TYPE (element)));

  gst_element_release_request_pad (GST_ELEMENT (GST_OBJECT_PARENT (rpad)),
      rpad);
}

static void
on_remove_ghost_pad (GSimpleAction * action,
    GVariant * parameter, gpointer user_data)
{
  GstEditorPadGhost *pad = GST_EDITOR_PAD_GHOST (user_data);
  GstPad *rpad = NULL;

  g_print ("deghost pad\n");

  rpad = GST_PAD (GST_EDITOR_ITEM (pad)->object);
  gst_element_remove_pad (GST_ELEMENT (GST_OBJECT_PARENT (rpad)), rpad);
}

static void
on_request_pad (GSimpleAction * action,
    GVariant * parameter, gpointer user_data)
{
  //GstEditorPadRequest *pad = GST_EDITOR_PAD_REQUEST (user_data);

  g_print ("request pad\n");
}

static void
on_frobate (GSimpleAction * action,
    GVariant * parameter, gpointer user_data)
{
  //GstEditorPadSometimes *pad = GST_EDITOR_PAD_SOMETIMES (user_data);
}

static void 
_gst_element_add_ghost_pad (GstElement * element, GstPad * pad,
    const gchar * name) 
{
  GstPad * ghostpad;
  g_return_if_fail (GST_IS_ELEMENT (element));
  g_return_if_fail (GST_IS_PAD (pad));
  
  /* then check to see if there's already a pad by that name here */ 
  g_return_if_fail (gst_object_check_uniqueness (element->pads, name) == TRUE);
  ghostpad = gst_ghost_pad_new (name, pad);
  gst_element_add_pad (element, ghostpad);
}

static void
on_ghost (GSimpleAction * action,
    GVariant * parameter, gpointer user_data)
{
  GstEditorPadAlways *pad = GST_EDITOR_PAD_ALWAYS (user_data);
  GstElement *bin;
  GstPad *p;

  p = GST_PAD (GST_EDITOR_ITEM (pad)->object);

  bin = (GstElement *) GST_OBJECT_PARENT (GST_OBJECT_PARENT (p));

  _gst_element_add_ghost_pad (bin, p, (gchar *) GST_OBJECT_NAME (p));
}

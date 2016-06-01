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


#include <gtk/gtk.h>

#include <gst/gst.h>
#include <gst/gstutils.h>

#include "editor.h"
#include "gst-helper.h"
#include "gsteditorproperty.h"
#include "gsteditoritem.h"
#include <gst/common/gste-marshal.h>
#include <gst/common/gste-debug.h>
#include "../../../pixmaps/pixmaps.h"

GST_DEBUG_CATEGORY (gste_element_debug);
#define GST_CAT_DEFAULT gste_element_debug

/* interface methods */
static void canvas_item_interface_init (GooCanvasItemIface * iface);
static gboolean gst_editor_element_button_press_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventButton * event);
static gboolean gst_editor_element_motion_notify_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventMotion * event);
static gboolean gst_editor_element_button_release_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventButton * event);

/* class methods */
static void gst_editor_element_class_init (GstEditorElementClass * klass);
static void gst_editor_element_init (GstEditorElement * element);
static void gst_editor_element_dispose (GObject * object);

static void gst_editor_element_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_editor_element_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

/*static*/ void gst_editor_element_realize (GooCanvasItem * citem);
static void gst_editor_element_resize (GstEditorItem * item);
static void gst_editor_element_repack (GstEditorItem * item);
static void gst_editor_element_object_changed (GstEditorItem * bin,
    GstObject * object);

/* events fired by items within self */
//static gint gst_editor_element_resizebox_event (GooCanvasItem * citem,
//    GdkEvent * event, GstEditorItem * item);
static gboolean
gst_editor_element_state_enter_notify_event (GooCanvasItem * item,
    GooCanvasItem * target_item, GdkEventCrossing * event, gpointer user_data);
static gboolean
gst_editor_element_state_leave_notify_event (GooCanvasItem * item,
    GooCanvasItem * target_item, GdkEventCrossing * event, gpointer user_data);
static gboolean
gst_editor_element_state_button_press_event (GooCanvasItem * item,
    GooCanvasItem * target_item, GdkEventButton * event, gpointer user_data);
static gboolean
gst_editor_element_state_button_release_event (GooCanvasItem * item,
    GooCanvasItem * target_item, GdkEventButton * event, gpointer user_data);

static gboolean gst_editor_element_resizebox_enter_notify_event (GooCanvasItem *
    item, GooCanvasItem * target_item, GdkEventCrossing * event,
    GstEditorItem * user_data);
static gboolean gst_editor_element_resizebox_leave_notify_event (GooCanvasItem *
    item, GooCanvasItem * target_item, GdkEventCrossing * event,
    GstEditorItem * user_data);
static gboolean gst_editor_element_resizebox_button_press_event (GooCanvasItem *
    item, GooCanvasItem * target_item, GdkEventButton * event,
    GstEditorItem * user_data);
static gboolean gst_editor_element_resizebox_button_release_event (GooCanvasItem
    * item, GooCanvasItem * target_item, GdkEventButton * event,
    GstEditorItem * user_data);
static gboolean gst_editor_element_resizebox_motion_notify_event (GooCanvasItem
    * item, GooCanvasItem * target_item, GdkEventMotion * event,
    GstEditorItem * user_data);


/* callbacks on the GstElement */
static void on_new_pad (GstElement * element, GstPad * pad,
    GstEditorElement * editor_element);
static void on_pad_removed (GstElement * element, GstPad * pad,
    GstEditorElement * editor_element);
//static void on_state_change (GstElement * element, GstState old,
//    GstState state, GstEditorElement * editor_element);
static void on_parent_unset (GstElement * element, GstBin * parent,
    GstEditorElement * editor_element);

static gboolean on_state_change (GstBus * bus, GstMessage * message,
    gpointer data);

/* utility functions */
static void gst_editor_element_add_pad (GstEditorElement * element,
    GstPad * pad);
static void gst_editor_element_remove_pad (GstEditorElement * element,
    GstPad * pad);

static void gst_editor_element_set_state (GstEditorElement * element,
    GstState state);
static gboolean gst_editor_element_set_state_cb (GstEditorElement * element);

/*static gboolean gst_editor_element_sync_state (GstEditorElement * element);*/

/* callbacks for the popup menu */
static void on_copy (GtkWidget * unused, GstEditorElement * element);
static void on_cut (GtkWidget * unused, GstEditorElement * element);
static void on_remove (GtkWidget * unused, GstEditorElement * element);


static GstState _gst_element_states[] = {
  GST_STATE_NULL,
  GST_STATE_READY,
  GST_STATE_PAUSED,
  GST_STATE_PLAYING,
};

enum
{
  ARG_0,
  ARG_ACTIVE,
  ARG_RESIZEABLE,
  ARG_MOVEABLE
};

enum
{
  SIZE_CHANGED,
  LAST_SIGNAL
};


static GObjectClass *parent_class;

static guint gst_editor_element_signals[LAST_SIGNAL] = { 0 };

#ifdef POPUP_MENU
static GnomeUIInfo menu_items[] = {
  GNOMEUIINFO_MENU_COPY_ITEM (on_copy, NULL),
  GNOMEUIINFO_MENU_CUT_ITEM (on_cut, NULL),
  GNOMEUIINFO_ITEM_STOCK ("_Remove element", "Remove element from bin",
      on_remove, "gtk-remove"),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_END
};
#endif

static const GtkActionEntry action_entries[] = {
  {"copy", "gtk-copy", NULL, NULL, NULL, G_CALLBACK (on_copy)},
  {"cut", "gtk-cut", NULL, NULL, NULL, G_CALLBACK (on_cut)},
  {"remove", "gtk-remove", "_Remove element", NULL, "Remove element from bin",
        G_CALLBACK (on_remove)},
};

static const char *ui_description =
    "<ui>"
    "  <popup name='itemMenu'>"
    "    <separator position='top' />"
    "    <menuitem name='remove' position='top' action='remove'/>"
    "    <menuitem name='cut' position='top' action='cut'/>"
    "    <menuitem name='copy' position='top' action='copy' />"
    "  </popup>" "</ui>";

GType
gst_editor_element_get_type (void)
{
  static GType element_type = 0;

  if (element_type == 0) {
    static const GTypeInfo element_info = {
      sizeof (GstEditorElementClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gst_editor_element_class_init,
      NULL,
      NULL,
      sizeof (GstEditorElement),
      0,
      (GInstanceInitFunc) gst_editor_element_init,
    };

    static const GInterfaceInfo iitem_info = {
      (GInterfaceInitFunc) canvas_item_interface_init,  /* interface_init */
      NULL,                     /* interface_finalize */
      NULL                      /* interface_data */
    };

    element_type =
        g_type_register_static (GST_TYPE_EDITOR_ITEM, "GstEditorElement",
        &element_info, 0);

    g_type_add_interface_static (element_type,
        GOO_TYPE_CANVAS_ITEM, &iitem_info);
  }
  return element_type;
}

static void
gst_editor_element_class_init (GstEditorElementClass * klass)
{
  GError *error = NULL;
  GObjectClass *object_class;
  GstEditorItemClass *item_class;


  object_class = G_OBJECT_CLASS (klass);
  item_class = GST_EDITOR_ITEM_CLASS (klass);

  parent_class = g_type_class_ref (GST_TYPE_EDITOR_ITEM);

  gst_editor_element_signals[SIZE_CHANGED] =
      g_signal_new ("size_changed", G_OBJECT_CLASS_TYPE (klass),
      G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GstEditorElementClass, size_changed),
      NULL, NULL, gst_editor_marshal_VOID__VOID, G_TYPE_NONE, 0);

  object_class->set_property = gst_editor_element_set_property;
  object_class->get_property = gst_editor_element_get_property;
  object_class->dispose = gst_editor_element_dispose;

  g_object_class_install_property (object_class, ARG_ACTIVE,
      g_param_spec_boolean ("active", "active", "active",
          FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, ARG_RESIZEABLE,
      g_param_spec_boolean ("resizeable", "resizeable", "resizeable",
          FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, ARG_MOVEABLE,
      g_param_spec_boolean ("moveable", "moveable", "moveable",
          TRUE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

#if 0
  citem_class->realize = gst_editor_element_realize;
#endif
  item_class->resize = gst_editor_element_resize;
  item_class->repack = gst_editor_element_repack;
  item_class->object_changed = gst_editor_element_object_changed;

#ifdef POPUP_MENU
  GST_EDITOR_ITEM_CLASS_PREPEND_MENU_ITEMS (item_class, menu_items, 4);
#endif
  GST_EDITOR_ITEM_CLASS_PREPEND_ACTION_ENTRIES (item_class, action_entries, 3);
  gtk_ui_manager_add_ui_from_string (item_class->ui_manager, ui_description, -1,
      &error);

  GST_DEBUG_CATEGORY_INIT (gste_element_debug, "GSTE_ELEMENT", 0,
      "GStreamer Editor Element Model");
}

static void
canvas_item_interface_init (GooCanvasItemIface * iface)
{
  iface->button_press_event = gst_editor_element_button_press_event;
  iface->motion_notify_event = gst_editor_element_motion_notify_event;
  iface->button_release_event = gst_editor_element_button_release_event;
}


static void
gst_editor_element_init (GstEditorElement * element)
{
  GstEditorItem *item = GST_EDITOR_ITEM (element);

  item->fill_color = 0xffffffff;
  item->outline_color = 0x333333ff;

  element->active = FALSE;

  element->next_state = GST_STATE_VOID_PENDING;
  element->set_state_idle_id = 0;
  
  g_static_rw_lock_init (&element->rwlock);

}

static void
gst_editor_element_dispose (GObject * object)
{
  GstEditorElement *element = GST_EDITOR_ELEMENT (object);

  if (element->set_state_idle_id) {
    g_source_remove (element->set_state_idle_id);
    element->set_state_idle_id = 0;
  }
  element->next_state = GST_STATE_VOID_PENDING;
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_editor_element_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstEditorElement *element = GST_EDITOR_ELEMENT (object);

  switch (prop_id) {
    case ARG_ACTIVE:
      element->active = g_value_get_boolean (value);
      g_object_set (G_OBJECT (GST_EDITOR_ITEM (element)->border),
          "line-width", (element->active ? 2.0 : 1.0), NULL);
      g_object_set (G_OBJECT (element->statebox),
          "line-width", (element->active ? 2.0 : 1.0), NULL);
      break;
    case ARG_RESIZEABLE:
      element->resizeable = g_value_get_boolean (value);
      if (!GST_EDITOR_ITEM (element)->realized)
        break;
      if (element->resizeable)
        goo_canvas_item_simple_show (GOO_CANVAS_ITEM_SIMPLE (element->
                resizebox));
      else
        goo_canvas_item_simple_hide (GOO_CANVAS_ITEM_SIMPLE (element->
                resizebox));
      break;
    case ARG_MOVEABLE:
      element->moveable = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_editor_element_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstEditorElement *element = GST_EDITOR_ELEMENT (object);

  switch (prop_id) {
    case ARG_ACTIVE:
      g_value_set_boolean (value, element->active);
      break;
    case ARG_RESIZEABLE:
      g_value_set_boolean (value, element->resizeable);
      break;
    case ARG_MOVEABLE:
      g_value_set_boolean (value, element->moveable);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/*static*/ void
gst_editor_element_realize (GooCanvasItem * citem)
{

  GstEditorElement *element;
  GstEditorItem *item;
  gint i;
  GdkPixbuf *pixbuf;
  static const guint8 *state_icons[] = {
    off_stock_image,
    on_stock_image,
    pause_stock_image,
    play_stock_image
  };

  element = GST_EDITOR_ELEMENT (citem);
  item = GST_EDITOR_ITEM (citem);

  g_static_rw_lock_reader_lock (&element->rwlock);
  //g_print("realize called for GooCanvas Item Pointer %p, Item-Pointer %p gotten mutex\n",(void*)citem, item);
  g_return_if_fail (GST_IS_EDITOR_ELEMENT (element));//if it has been deleted we dont touch it...

  gst_editor_item_realize (citem);
#if 0
  if (GNOME_CANVAS_ITEM_CLASS (parent_class)->realize)
    GNOME_CANVAS_ITEM_CLASS (parent_class)->realize (citem);
#endif

  /* the resize box */
  element->resizebox = goo_canvas_rect_new (GOO_CANVAS_ITEM (citem),
      0., 0., 0., 0.,
      "line-width", 1.,
      "fill_color", "white", "stroke_color", "black", "antialias",
      CAIRO_ANTIALIAS_NONE, NULL);
  g_return_if_fail (element->resizebox != NULL);
  GST_EDITOR_SET_OBJECT (element->resizebox, item);

  g_signal_connect (G_OBJECT (element->resizebox), "enter-notify-event",
      G_CALLBACK (gst_editor_element_resizebox_enter_notify_event), element);
  g_signal_connect (G_OBJECT (element->resizebox), "leave-notify-event",
      G_CALLBACK (gst_editor_element_resizebox_leave_notify_event), element);
  g_signal_connect (G_OBJECT (element->resizebox), "button-press-event",
      G_CALLBACK (gst_editor_element_resizebox_button_press_event), element);
  g_signal_connect (G_OBJECT (element->resizebox), "motion-notify-event",
      G_CALLBACK (gst_editor_element_resizebox_motion_notify_event), element);
  g_signal_connect (G_OBJECT (element->resizebox), "button-release-event",
      G_CALLBACK (gst_editor_element_resizebox_button_release_event), element);

  if (!element->resizeable)
    goo_canvas_item_simple_hide (GOO_CANVAS_ITEM_SIMPLE (element->resizebox));

  /* create the state boxen */
  element->statebox = goo_canvas_rect_new (GOO_CANVAS_ITEM (item),
      0., 0., 0., 0.,
      "line-width", 1.,
      "fill_color", "white", "stroke_color", "black", "antialias",
      CAIRO_ANTIALIAS_NONE, NULL);
  g_return_if_fail (element->statebox != NULL);

  GST_EDITOR_SET_OBJECT (element->statebox, element);

  for (i = 0; i < 4; i++) {
    pixbuf = gdk_pixbuf_new_from_inline (-1, state_icons[i], FALSE, NULL);
    element->stateicons[i] = goo_canvas_image_new (GOO_CANVAS_ITEM (item),
        pixbuf, 0.0, 0.0, "antialias", CAIRO_ANTIALIAS_NONE, NULL);

    GST_EDITOR_SET_OBJECT (element->stateicons[i], element);

    g_signal_connect (element->stateicons[i], "enter-notify-event",
        G_CALLBACK (gst_editor_element_state_enter_notify_event),
        GINT_TO_POINTER (i));
    g_signal_connect (element->stateicons[i], "leave-notify-event",
        G_CALLBACK (gst_editor_element_state_leave_notify_event),
        GINT_TO_POINTER (i));
    g_signal_connect (element->stateicons[i], "button-press-event",
        G_CALLBACK (gst_editor_element_state_button_press_event),
        GINT_TO_POINTER (i));
    g_signal_connect (element->stateicons[i], "button-release-event",
        G_CALLBACK (gst_editor_element_state_button_release_event),
        GINT_TO_POINTER (i));
  }

  gst_editor_element_add_pads (element);

  item->realized = TRUE;

  if (G_OBJECT_TYPE (item) == GST_TYPE_EDITOR_ELEMENT) gst_editor_element_move(element,0.0,0.0);


  if (G_OBJECT_TYPE (item) == GST_TYPE_EDITOR_ELEMENT)
    gst_editor_item_resize (item);
  g_static_rw_lock_reader_unlock (&element->rwlock);

}

static void
gst_editor_element_resize (GstEditorItem * item)
{
  GstEditorItem *subitem;
  GstEditorElement *element;
  GList *l;

  element = GST_EDITOR_ELEMENT (item);

  /* a little bit of padding, eh */
  item->l.h += 4.0;
  item->r.h += 4.0;

  /* the resize box is 4x4 */
  item->b.w += 4.0;
  item->b.h = MAX (item->b.h, 4.0);

  /* state boxes are 16.0 x 16.0 + 2 px for the border */
  element->statewidth = 18.0;
  element->stateheight = 18.0;
  item->b.w += element->statewidth * 4 + 2.0;
  /* 1 px of vertical padding.. */
  item->b.h = MAX (item->b.h, element->stateheight + 1.0);

  /* now go and try to calculate necessary space for the pads */
  element->sinkwidth = 0.0;
  element->sinkheight = 0.0;
  element->sinks = 0;
  for (l = element->sinkpads; l; l = l->next) {
    subitem = GST_EDITOR_ITEM (l->data);
    element->sinkwidth = MAX (element->sinkwidth, subitem->width);
    element->sinkheight = MAX (element->sinkheight, subitem->height);
    element->sinks++;
  }
  item->l.w = MAX (item->l.w, element->sinkwidth + 12.0);
  item->l.h += element->sinkheight * element->sinks;

  element->srcwidth = 0.0;
  element->srcheight = 0.0;
  element->srcs = 0;
  for (l = element->srcpads; l; l = l->next) {
    subitem = GST_EDITOR_ITEM (l->data);
    element->srcwidth = MAX (element->srcwidth, subitem->width);
    element->srcheight = MAX (element->srcheight, subitem->height);
    element->srcs++;
  }
  item->r.w = MAX (item->r.w, element->srcwidth + 12.0);
  item->r.h += element->srcheight * element->srcs;

  GST_EDITOR_ITEM_CLASS (parent_class)->resize (item);
}

static void
gst_editor_element_repack (GstEditorItem * item)
{
  GstEditorElement *element;
  GList *l;
  GstEditorItem *subitem;
  gint sinks;
  gint srcs;
  gdouble x1, y1, x2, y2;
  gint i;

  if (!item->realized)
    return;

  element = GST_EDITOR_ELEMENT (item);

  /* the resize box */
//  gnome_canvas_item_set (element->resizebox,
//      "x1", item->width - 4.0,
//      "y1", item->height - 4.0, "x2", item->width, "y2", item->height, NULL);
  g_object_set (element->resizebox, "x", item->width - 4.0, "y",
      item->height - 4.0, "width", 4.0, "height", 4.0, NULL);

  /* make sure args to gnome_canvas_item_set are doubles */
  x1 = 0.0;
  y1 = 0.0;
  x2 = item->width;
  y2 = item->height;

  /* place the state boxes */
  for (i = 0; i < 4; i++) {
    g_return_if_fail (element->stateicons[i] != NULL);
    g_object_set (element->stateicons[i],
        "x", x1 + (element->statewidth * i) + 1.0,
        "y", y2 - element->stateheight + 1.0, NULL);
  }
  //g_print("syncing from repack: ");
  //gst_editor_element_sync_state (element);
  g_idle_add ((GSourceFunc) gst_editor_element_sync_state, element);
  /* place the pads */
  sinks = element->sinks;
  l = element->sinkpads;
  while (l) {
    subitem = GST_EDITOR_ITEM (l->data);

    goo_canvas_item_set_simple_transform (GOO_CANVAS_ITEM (subitem),
        x1 + 1.0,
        y2 - 3.0 - element->stateheight - (element->sinkheight * sinks), 1.,
        0.);

    sinks--;
    l = g_list_next (l);
  }
  srcs = element->srcs;
  l = element->srcpads;
  while (l) {
    subitem = GST_EDITOR_ITEM (l->data);

    goo_canvas_item_set_simple_transform (GOO_CANVAS_ITEM (subitem),
        x2 - GST_EDITOR_ITEM (subitem)->width,
        y2 - 3.0 - element->stateheight - (element->srcheight * srcs), 1., 0.);

    srcs--;
    l = g_list_next (l);
  }

  if (GST_EDITOR_ITEM_CLASS (parent_class)->repack)
    (GST_EDITOR_ITEM_CLASS (parent_class)->repack) (item);
  //g_print("survived repack");
}

static gboolean
gst_editor_element_button_press_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventButton * event)
{
  GooCanvasItemIface *iface;
  GdkCursor *fleur;
  GstEditorElement *element = GST_EDITOR_ELEMENT (citem);

//    g_print ("%p received 'button-press' signal at %g, %g (root: %g, %g)\n",
//         citem, event->x, event->y, event->x_root, event->y_root);

  if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
//     g_object_set (citem->canvas, "selection", element, NULL);
    g_object_set (goo_canvas_item_get_canvas (citem), "selection", element,
        NULL);

    if (element->moveable) {
      /* dragxy coords are world coords of button press */
      element->dragx = event->x;
      element->dragy = event->y;
      /* set some flags */
      element->dragging = TRUE;
      element->moved = FALSE;
      fleur = gdk_cursor_new (GDK_FLEUR);

      goo_canvas_pointer_grab (goo_canvas_item_get_canvas (citem),
          citem, GDK_POINTER_MOTION_MASK |
/*                             GDK_ENTER_NOTIFY_MASK | */
/*                             GDK_LEAVE_NOTIFY_MASK | */
          GDK_BUTTON_RELEASE_MASK, fleur, event->time);
      gdk_cursor_unref (fleur);
    }

    return TRUE;
  }
//   if (GOO_CANVAS_ITEM_GET_IFACE (goo_canvas_item_get_parent(citem))->button_press_event)
//    return GOO_CANVAS_ITEM_GET_IFACE (goo_canvas_item_get_parent(citem))->button_press_event (citem, target, event);
  iface =
      (GooCanvasItemIface *) g_type_interface_peek (parent_class,
      GOO_TYPE_CANVAS_ITEM);
  if (iface->button_press_event)
    return iface->button_press_event (citem, target, event);

  return FALSE;
}

static gboolean
gst_editor_element_motion_notify_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventMotion * event)
{
  gdouble dx, dy;

  GstEditorElement *element = GST_EDITOR_ELEMENT (citem);

  if (element->dragging) {

//   g_print ("%p received 'move' signal at %g, %g (root: %g, %g)\n",
//         citem, event->x, event->y, event->x_root, event->y_root);

    dx = event->x - element->dragx;
    dy = event->y - element->dragy;
    gst_editor_element_move (element, dx, dy);
//     element->dragx = event->x;
//     element->dragy = event->y;
//       g_print ("%p received 'moved' at %g, %g \n", citem, dx, dy);
    element->moved = TRUE;
  }
  return TRUE;
}

static gboolean
gst_editor_element_button_release_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventButton * event)
{
  GstEditorElement *element = GST_EDITOR_ELEMENT (citem);

  if (element->dragging /*&& event->button == 1 */ ) {
    element->dragging = FALSE;
//     gnome_canvas_item_ungrab (citem, event->time);
    goo_canvas_pointer_ungrab (goo_canvas_item_get_canvas (citem),
        citem, event->time);
    return TRUE;
  }
//   if (GOO_CANVAS_ITEM_GET_IFACE (goo_canvas_item_get_parent(citem))->button_release_event)
//      return GOO_CANVAS_ITEM_GET_IFACE (goo_canvas_item_get_parent(citem))->button_release_event (citem, target, event);

  return FALSE;
}

static void
gst_editor_element_object_changed (GstEditorItem * item, GstObject * object)
{
  if (item->object) {
//    g_signal_handlers_disconnect_by_func (G_OBJECT (item->object),
//        on_state_change, item);
    // g_source_remove                     (guint tag);
    g_signal_handlers_disconnect_by_func (G_OBJECT (item->object), on_new_pad,
        item);
    g_signal_handlers_disconnect_by_func (G_OBJECT (item->object),
        on_pad_removed, item);
    g_signal_handlers_disconnect_by_func (G_OBJECT (item->object),
        on_parent_unset, item);
    //in case it stays alive..
    //if(G_IS_OBJECT(item)&&G_OBJECT_PARENT(item)&&GST_IS_EDITOR_BIN(G_OBJECT_PARENT(item))){
    //  GstEditorBin* parent=GST_EDITOR_BIN(G_OBJECT_PARENT(item));
    //  g_signal_handlers_disconnect_by_func (G_OBJECT (item->object),
    //    G_CALLBACK (gst_editor_bin_element_added_cb), parent);
    //}
  }

  if (object) {
//    g_signal_connect (G_OBJECT (object), "state-change",
//        G_CALLBACK (on_state_change), item);

//    GstElement *elt = GST_ELEMENT(object);
//    GstBus *bus = gst_element_get_bus(elt);
//    gst_bus_add_watch(bus, &on_state_change, NULL/*item*/);

    g_signal_connect (G_OBJECT (object), "pad-added", G_CALLBACK (on_new_pad),
        item);
    g_signal_connect (G_OBJECT (object), "pad-removed",
        G_CALLBACK (on_pad_removed), item);
    g_signal_connect (G_OBJECT (object), "parent-unset",
        G_CALLBACK (on_parent_unset), item);
  }

  if (GST_EDITOR_ITEM_CLASS (parent_class)->object_changed)
    (GST_EDITOR_ITEM_CLASS (parent_class)->object_changed) (item, object);
}

/**********************************************************************
 * Events from canvasitems internal to the editor element
 **********************************************************************/
#if 0
static gint
gst_editor_element_resizebox_event (GooCanvasItem * citem,
    GdkEvent * event, GstEditorItem * item)
{
  GstEditorElement *element;
  GdkCursor *bottomright;
  gdouble item_x, item_y;

  element = GST_EDITOR_ELEMENT (item);

  /* calculate coords relative to the group, not the box */
  item_x = event->button.x;
  item_y = event->button.y;
//  gnome_canvas_item_w2i (citem->parent, &item_x, &item_y);
  goo_canvas_convert_to_item_space (goo_canvas_item_get_canvas (citem),
      goo_canvas_item_get_parent (citem), &item_x, &item_y);

  switch (event->type) {
    case GDK_ENTER_NOTIFY:
      g_object_set (G_OBJECT (element->resizebox), "fill_color", "red", NULL);
      return TRUE;
      break;
    case GDK_LEAVE_NOTIFY:
      g_object_set (G_OBJECT (element->resizebox), "fill_color", "white", NULL);
      element->hesitating = FALSE;
      return TRUE;
      break;
    case GDK_BUTTON_PRESS:
      element->dragx = event->button.x;
      element->dragy = event->button.y;
      element->resizing = TRUE;
      element->hesitating = TRUE;
      bottomright = gdk_cursor_new (GDK_BOTTOM_RIGHT_CORNER);
      goo_canvas_pointer_grab (goo_canvas_item_get_canvas (citem), citem,
          GDK_POINTER_MOTION_MASK |
          GDK_ENTER_NOTIFY_MASK |
          GDK_LEAVE_NOTIFY_MASK |
          GDK_BUTTON_RELEASE_MASK, bottomright, event->button.time);
      return TRUE;
      break;
    case GDK_MOTION_NOTIFY:
      if (element->resizing) {
        if (item_x > 0.0 && item_y > 0.0)
          g_object_set (G_OBJECT (element), "width", item_x, "height", item_y,
              NULL);
        return TRUE;
      }
      break;
    case GDK_BUTTON_RELEASE:
      if (element->resizing) {
        element->resizing = FALSE;
        goo_canvas_pointer_ungrab (goo_canvas_item_get_canvas (citem),
            citem, event->button.time);
        return TRUE;
      }
      break;
    default:
      break;
  }
  return FALSE;
}
#endif
static gboolean
gst_editor_element_resizebox_enter_notify_event (GooCanvasItem * citem,
    GooCanvasItem * target_item, GdkEventCrossing * event, GstEditorItem * item)
{
  GstEditorElement *element;

  element = GST_EDITOR_ELEMENT (item);

  g_object_set (G_OBJECT (element->resizebox), "fill_color", "red", NULL);
  return TRUE;
}

static gboolean
gst_editor_element_resizebox_leave_notify_event (GooCanvasItem * citem,
    GooCanvasItem * target_item, GdkEventCrossing * event, GstEditorItem * item)
{
  GstEditorElement *element;

  element = GST_EDITOR_ELEMENT (item);

  g_object_set (G_OBJECT (element->resizebox), "fill_color", "white", NULL);
  element->hesitating = FALSE;

  return TRUE;
}

static gboolean
gst_editor_element_resizebox_button_press_event (GooCanvasItem * citem,
    GooCanvasItem * target_item, GdkEventButton * event, GstEditorItem * item)
{
  GstEditorElement *element;
  GdkCursor *bottomright;

  element = GST_EDITOR_ELEMENT (item);

  element->dragx = event->x;
  element->dragy = event->y;
  element->resizing = TRUE;
  element->hesitating = TRUE;
  bottomright = gdk_cursor_new (GDK_BOTTOM_RIGHT_CORNER);

  goo_canvas_pointer_grab (goo_canvas_item_get_canvas (citem), citem,
      GDK_POINTER_MOTION_MASK |
      GDK_ENTER_NOTIFY_MASK |
      GDK_LEAVE_NOTIFY_MASK |
      GDK_BUTTON_RELEASE_MASK, bottomright, event->time);

  return TRUE;
}

static gboolean
gst_editor_element_resizebox_button_release_event (GooCanvasItem * citem,
    GooCanvasItem * target_item, GdkEventButton * event, GstEditorItem * item)
{
  GstEditorElement *element;

  element = GST_EDITOR_ELEMENT (item);

  if (element->resizing) {
    element->resizing = FALSE;
    goo_canvas_pointer_ungrab (goo_canvas_item_get_canvas (citem),
        citem, event->time);
    return TRUE;
  }

  return FALSE;
}

static gboolean
gst_editor_element_resizebox_motion_notify_event (GooCanvasItem * citem,
    GooCanvasItem * target_item, GdkEventMotion * event, GstEditorItem * item)
{
  GstEditorElement *element;
  gdouble item_x, item_y;

  element = GST_EDITOR_ELEMENT (item);

//   /* calculate coords relative to the group, not the box */
  item_x = event->x;
  item_y = event->y;
//  gnome_canvas_item_w2i (citem->parent, &item_x, &item_y);
//   goo_canvas_convert_to_item_space(goo_canvas_item_get_canvas (citem),
//      goo_canvas_item_get_parent(citem), &item_x, &item_y);

  if (element->resizing) {
    if (item_x > 0.0 && item_y > 0.0)
      g_object_set (G_OBJECT (element), "width", item_x, "height", item_y,
          NULL);

    return TRUE;
  }

  return FALSE;
}


static gboolean
gst_editor_element_state_enter_notify_event (GooCanvasItem * citem,
    GooCanvasItem * target_item, GdkEventCrossing * event, gpointer user_data)
{
  GdkCursor *uparrow;

  uparrow = gdk_cursor_new (GDK_SB_UP_ARROW);

  goo_canvas_pointer_grab (goo_canvas_item_get_canvas (citem),
      citem,
      GDK_POINTER_MOTION_MASK |
      GDK_BUTTON_RELEASE_MASK | GDK_LEAVE_NOTIFY_MASK, uparrow, event->time);

  gdk_cursor_unref (uparrow);
  /* NOTE: when grabbing canvas item, always get pointer_motion,
     this will allow you to actually get all the other synth events */

  return FALSE;
}

static gboolean
gst_editor_element_state_leave_notify_event (GooCanvasItem * citem,
    GooCanvasItem * target_item, GdkEventCrossing * event, gpointer user_data)
{
  //      gnome_canvas_item_ungrab (citem, event->button.time);
  goo_canvas_pointer_ungrab (goo_canvas_item_get_canvas (citem),
      citem, event->time);

  return FALSE;
}

static gboolean
gst_editor_element_state_button_press_event (GooCanvasItem * item,
    GooCanvasItem * target_item, GdkEventButton * event, gpointer user_data)
{
  if (event->button == 1)
    return TRUE;
  else
    return FALSE;
}

void
gst_editor_element_stop_child (GstEditorElement * child)
{
  if GST_IS_EDITOR_BIN(child){//make this recursive to work for bin inside bin too
        GstEditorBin *bin=GST_EDITOR_BIN(child);
        GList *children;
        children = bin->elements;
        g_list_foreach (children, (GFunc) gst_editor_element_stop_child, NULL);
        }
  g_idle_add ((GSourceFunc) gst_editor_element_sync_state, &(child->item));
}


static gboolean
gst_editor_element_state_button_release_event (GooCanvasItem * citem,
    GooCanvasItem * target_item, GdkEventButton * event, gpointer user_data)
{
  GstEditorElement *element;
  GstEditorBin *bin;
  gint id = GPOINTER_TO_INT (user_data);

  element = GST_EDITOR_GET_OBJECT (citem);
  if (element && GST_IS_EDITOR_BIN(element)){
     bin = GST_EDITOR_GET_OBJECT (citem);
  } else bin=NULL;

  if (event->button != 1)
    return FALSE;
  //g_print("Button released %d\n",id);
  if (id == 0 && bin != NULL)   // it needs extra handling as there will be no message on the bus on change to NULL state...
  {
    GList *children;
    children = bin->elements;
    g_list_foreach (children, (GFunc) gst_editor_element_stop_child, NULL);
    gst_element_set_state (GST_ELEMENT (element->item.object), GST_STATE_NULL);
    g_idle_add ((GSourceFunc) gst_editor_element_sync_state, &(element->item));
//  } else if (id == 0 && bin == NULL){//so any DAU hit stop directly on the element...
//    
  } else if (id < 4) {
    element->next_state = _gst_element_states[id];
    if (element->set_state_idle_id == 0){
      //g_print("will now add idle function\n");
      element->set_state_idle_id =
          g_idle_add ((GSourceFunc) (gst_editor_element_set_state_cb), element);
    }
    /* Release the mouse grab. This is a hack to avoid the editor locking up on state change, which it 
     * does a lot at the moment.
     */
//        gnome_canvas_item_ungrab (citem, event->button.time);
    goo_canvas_pointer_ungrab (goo_canvas_item_get_canvas (citem),
        citem, event->time);
  } else
    GST_CAT_WARNING (gste_debug_cat, "Attempted to set unknown state id %d",
        id);
  //g_print("return from button released\n");
  return TRUE;
}

/**********************************************************************
 * Callbacks from the gstelement (must be threadsafe)
 **********************************************************************/

static void
on_new_pad (GstElement * element, GstPad * pad,
    GstEditorElement * editor_element)
{
  g_static_rw_lock_writer_lock (GST_EDITOR_ITEM(editor_element)->globallock);
  GST_CAT_DEBUG (gste_debug_cat, "new_pad in element %s\n",
      GST_OBJECT_NAME (element));

  gst_editor_element_add_pad (editor_element, pad);
  gst_editor_item_resize (GST_EDITOR_ITEM (editor_element));
  gst_editor_element_move(editor_element,0.0,0.0);
  g_static_rw_lock_writer_unlock (GST_EDITOR_ITEM(editor_element)->globallock);  
}

static void
on_pad_removed (GstElement * element, GstPad * pad,
    GstEditorElement * editor_element)
{
  g_static_rw_lock_writer_lock (GST_EDITOR_ITEM(editor_element)->globallock);
  GST_CAT_DEBUG (gste_debug_cat, "pad_removed in element %s\n",
      GST_OBJECT_NAME (element));

  gst_editor_element_remove_pad (editor_element, pad);
  gst_editor_item_resize (GST_EDITOR_ITEM (editor_element));
  g_static_rw_lock_writer_unlock (GST_EDITOR_ITEM(editor_element)->globallock);  
}

#if 0
static void
on_state_change (GstElement * element, GstState old,
    GstState state, GstEditorElement * editor_element)
{
/* FIXME
   if (state == GST_STATE_PLAYING &&
      GST_IS_BIN (element) &&
      GST_FLAG_IS_SET (element, GST_BIN_FLAG_MANAGER) &&
      !GST_FLAG_IS_SET (element, GST_BIN_SELF_SCHEDULABLE)) {
    EDITOR_DEBUG ("Adding iterator for pipeline");
    if (!editor_element->source)
      editor_element->source =
          g_idle_add ((GSourceFunc) gst_bin_iterate, element);
  } else if (editor_element->source) {
    EDITOR_DEBUG ("Removing iterator for pipeline");
    g_source_remove (editor_element->source);
    editor_element->source = 0;
  }
*/
  g_idle_add ((GSourceFunc) gst_editor_element_sync_state, editor_element);
}
#else
static gboolean
on_state_change (GstBus * bus, GstMessage * message, gpointer data)
{
  /*GstEditorElement *editor_element = GST_EDITOR_ELEMENT(data); */
  g_print("On state changed called!\n");
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_STATE_CHANGED:
    {
      GstEditorItem *item =
          gst_editor_item_get (GST_OBJECT (GST_MESSAGE_SRC (message)));
      g_idle_add ((GSourceFunc) gst_editor_element_sync_state,
          item /*editor_element */ );
      break;
    }
    default:
      /* unhandled message */
      break;
  }

  /* remove message from the queue */
  return TRUE;
}
#endif
//method to kick out all children and their children but not the element itself,
//hack out of hashtable
void unset_deepdelete(GooCanvasItem * citem){
  int numc=goo_canvas_item_get_n_children(citem); 
  g_print("unset:deepdelete removing %d children\n",numc);
  for (numc--;numc>0;numc--){
    GooCanvasItem * temp;
    temp=goo_canvas_item_get_child(citem,numc);
    unset_deepdelete(temp);
    //if (GST_IS_EDITOR_ELEMENT(temp)) unset_deepdelete(temp);
    //else if (GST_IS_EDITOR_PAD(temp))
    if (GST_IS_EDITOR_ITEM(temp)){
        g_print("Unset deepdelete, processing %s.\n",GST_EDITOR_ITEM(temp)->title_text);
	gst_editor_item_hash_remove (GST_EDITOR_ITEM(temp)->object);
        //gst_editor_item_dispose(GST_EDITOR_ITEM(temp));
        g_object_set(GST_EDITOR_ITEM(temp),"object",NULL,NULL);//should free all the callbacks
        GST_EDITOR_ITEM(temp)->title=NULL;
        GST_EDITOR_ITEM(temp)->border=NULL;
	//g_object_unref(GST_EDITOR_ITEM(temp));
        
    }
    goo_canvas_item_remove_child(citem,numc);
  }
}

void unset_deepdelete_editor_pads(GstEditorElement * element){
  int numc=g_list_length(element->srcpads); 
  g_print("removing %d srcpads from element %p\n",numc,element);
  GList *l;
  l=element->srcpads;
  while (l){
  if (GST_IS_EDITOR_ITEM(l->data)){
	gst_editor_item_hash_remove (GST_EDITOR_ITEM(l->data)->object);
        g_object_set(GST_EDITOR_ITEM(l->data),"object",NULL,NULL);//should free all the callbacks
  }
  if (GOO_IS_CANVAS_ITEM(l->data))goo_canvas_item_remove(l->data);
  l=g_list_next(l);
  }
  numc=g_list_length(element->sinkpads); 
  g_print("removing %d sinkpads\n",numc);
  l=element->sinkpads;
  while (l){
  if (GST_IS_EDITOR_ITEM(l->data)){
	g_print("Processing sinkpad %p, object %p\n",l->data,GST_EDITOR_ITEM(l->data)->object);
	gst_editor_item_hash_remove (GST_EDITOR_ITEM(l->data)->object);
        g_object_set(GST_EDITOR_ITEM(l->data),"object",NULL,NULL);//should free all the callbacks
    }
    g_print("deleting canvas next\n");
    if (GOO_IS_CANVAS_ITEM(l->data))goo_canvas_item_remove(l->data);
    l=g_list_next(l);
  }
  g_print("finished unset_deepdelete_editor_pads\n");
}

void unset_deepdelete_editor(GstEditorBin * bin){
  int numc=g_list_length(bin->elements); 
  g_print("removing %d children\n",numc);
  GList *l;
  l=bin->elements;
  while (l){
  if (GST_IS_EDITOR_BIN(l->data))unset_deepdelete_editor(l->data);
  if (GST_IS_EDITOR_ELEMENT(l->data)) unset_deepdelete_editor_pads(GST_EDITOR_ELEMENT(l->data));
  if (GST_IS_EDITOR_ITEM(l->data)){
	gst_editor_item_hash_remove (GST_EDITOR_ITEM(l->data)->object);
        gst_editor_item_disconnect(bin, l->data);
        g_object_set(GST_EDITOR_ITEM(l->data),"object",NULL,NULL);//should free all the callbacks
  }
  if (GOO_IS_CANVAS_ITEM(l->data))goo_canvas_item_remove(l->data);
  l=g_list_next(l);
  }
  l=bin->links;
  while (l){ 
  if ((l->data)&&(GST_IS_EDITOR_LINK(l->data))){
    if (GOO_IS_CANVAS_ITEM(GST_EDITOR_LINK(l->data)->canvas))	
	goo_canvas_item_remove(GST_EDITOR_LINK(l->data)->canvas);
    else g_print("GstEditorlink %p has no canvas\n",l->data);
    g_object_unref(l->data);
  }
  else g_print("failed to remove GstEditorlink %p\n",l->data);
  l=g_list_next(l);
  }
  g_print("unset_deepdelete finished\n");
}

static void
on_parent_unset (GstElement * element, GstBin * parent,
    GstEditorElement * editor_element)
{
  g_print("removing this item:%s Pointer:%p Getting global Mutex for write\n",GST_OBJECT_NAME(element),element);
  g_static_rw_lock_writer_lock (GST_EDITOR_ITEM(editor_element)->globallock);
  g_static_rw_lock_writer_lock (&editor_element->rwlock);

  GstEditorBin *editor_bin;

  editor_bin = GST_EDITOR_BIN (gst_editor_item_get (GST_OBJECT (parent)));

  /* unreffing groups does nothing to the children right now. grrrrr. just hide
   * the damn thing and hack out the editorbin internals */
  //goo_canvas_item_simple_hide (GOO_CANVAS_ITEM_SIMPLE (editor_element));
  
  if (editor_bin){
    gst_editor_item_disconnect(editor_bin, editor_element);
    editor_bin->elements = g_list_remove (editor_bin->elements, editor_element);
  }
  else g_print("fixme: on_parent_unset called on element %s. Parent unknown!\n",GST_OBJECT_NAME(element));

  if (editor_element->active)
    g_object_set (goo_canvas_item_get_canvas (GOO_CANVAS_ITEM (editor_element)),
        "selection", NULL, NULL);

  gst_editor_item_hash_remove(element);//for the save side twice
  if (GST_EDITOR_ITEM (editor_element)->object) {
  //  gst_editor_item_hash_remove(GST_EDITOR_ITEM (editor_element)->object);
    //g_object_unref(GST_EDITOR_ITEM (editor_element)->object);
    if (G_IS_OBJECT(editor_element)) g_object_set (GST_EDITOR_ITEM (editor_element), "object", NULL, NULL);
  }
  //kill children...
  if (GST_IS_EDITOR_BIN(editor_element)){
    unset_deepdelete_editor(GST_EDITOR_BIN(editor_element));
    //unset_deepdelete(GOO_CANVAS_ITEM(editor_element));
  }
  //
  goo_canvas_item_remove(GOO_CANVAS_ITEM(editor_element));
  g_static_rw_lock_writer_unlock (&editor_element->rwlock); 
  g_static_rw_lock_writer_unlock (GST_EDITOR_ITEM(editor_element)->globallock);
  g_object_unref (G_OBJECT (editor_element));
  g_print("Survived removing this item:%s %p Pointer of GstEditorElement %p\n",GST_OBJECT_NAME(element),element,editor_element);
}

/**********************************************************************
 * Utility functions
 **********************************************************************/

static void
gst_editor_element_add_pad (GstEditorElement * element, GstPad * pad)
{
//  GooCanvasBounds bounds;
  GstEditorItem *editor_pad;
  GstPadTemplate *template;
  GType pad_type;

  if (GST_IS_GHOST_PAD (pad))
    pad_type = gst_editor_pad_ghost_get_type ();
  else if ((template = GST_PAD_PAD_TEMPLATE (pad))
      && template->presence == GST_PAD_REQUEST)
    pad_type = gst_editor_pad_requested_get_type ();
  else
    pad_type = gst_editor_pad_always_get_type ();

  editor_pad =
//      GST_EDITOR_ITEM (gnome_canvas_item_new (GNOME_CANVAS_GROUP (element),
//          pad_type, "object", G_OBJECT (pad), NULL));
      GST_EDITOR_ITEM (goo_canvas_item_new (GOO_CANVAS_ITEM (element),
          pad_type, "object", G_OBJECT (pad),"globallock",(GST_EDITOR_ITEM(element))->globallock, NULL));
  gst_editor_pad_realize (editor_pad);
//  goo_canvas_item_get_bounds(GOO_CANVAS_ITEM (element), &bounds);
//  goo_canvas_item_translate(editor_pad, bounds.x1, bounds.y1);

  if (GST_PAD_DIRECTION (pad) == GST_PAD_SINK) {
    element->sinkpads = g_list_append (element->sinkpads, editor_pad);
    element->sinks++;
  } else {
    element->srcpads = g_list_append (element->srcpads, editor_pad);
    element->srcs++;
  }
  //gst_editor_element_repack(GST_EDITOR_ITEM(element));
}

static GstIteratorItem
iterate_pad (GstIterator * it, GstPad * pad)
{
  gst_object_ref (pad);
  return GST_ITERATOR_ITEM_PASS;
}

static GstIterator *
gst_element_iterate_pad_list (GstElement * element, GList ** padlist)
{
  g_return_val_if_fail (GST_IS_ELEMENT (element), NULL);
  GstIterator *result;

  GST_OBJECT_LOCK (element);
  gst_object_ref (element);
  result = gst_iterator_new_list (GST_TYPE_PAD,
      GST_OBJECT_GET_LOCK (element),
      &element->pads_cookie,
      padlist,
      element,
      (GstIteratorItemFunction) iterate_pad,
      (GstIteratorDisposeFunction) gst_object_unref);
  GST_OBJECT_UNLOCK (element);

  return result;
}


//TODO: declare static again...
void
gst_editor_element_add_pads (GstEditorElement * element)
{
  GstPad *pad;
  GstPadTemplate *pad_template;
  GList *pad_templates, *l, *w, *reversed;
  GstEditorItem *item = (GstEditorItem *) element, *editor_pad;
  GstElement *e;

  gboolean done = FALSE;
  GstIterator *pads;

  e = GST_ELEMENT (item->object);
  pad_templates =
      g_list_copy (gst_element_class_get_pad_template_list
      (GST_ELEMENT_GET_CLASS (e)));
  reversed=g_list_reverse(g_list_copy(e->pads));
  pads=gst_element_iterate_pad_list (e, &reversed);//to make it the right order
  //pads = gst_element_iterate_pads (e);
  while (!done) {
    gpointer item;

    switch (gst_iterator_next (pads, &item)) {
      case GST_ITERATOR_OK:
        pad = GST_PAD (item);
        pad_template = gst_pad_get_pad_template (pad);
        /* 
         * Go through the pad_templates list and remove the pad_template for
         * this pad, rather than show both the pad and pad_template.
	 * Comment of Hannes: Why should we do this!?!?! We maybe want to edit the graph!!!
	 * TODO: rewrite it instead of just commenting out the code
         */
        if (pad_template) {
          w = pad_templates;
          EDITOR_LOG ("Trying to find pad template %s\n",
              GST_OBJECT_NAME (pad_template));
          while (w) {
            if (g_ascii_strcasecmp (GST_OBJECT_NAME (w->data),
                    GST_OBJECT_NAME (pad_template)) == 0) {
              break;
            }
            w = g_list_next (w);
          }
          if (w) {
            //pad_templates = g_list_remove_link (pad_templates, w);
          }
        } else {
          EDITOR_LOG ("Element %s: pad '%s' has no pad template",
              g_type_name (G_OBJECT_TYPE (e)), GST_OBJECT_NAME (pad));
        }


        EDITOR_DEBUG ("adding pad %s to element %s",
            GST_OBJECT_NAME (pad), gst_element_get_name (e));
        gst_editor_element_add_pad (element, pad);

        gst_object_unref (pad);
        break;
      case GST_ITERATOR_RESYNC:
        //...rollback changes to items...
        gst_iterator_resync (pads);
        break;
      case GST_ITERATOR_ERROR:
        //...wrong parameter were given...
      case GST_ITERATOR_DONE:
      default:
        done = TRUE;
        break;
    }
  }
  gst_iterator_free (pads);
  g_list_free (reversed);


  for (l = g_list_last(pad_templates); l; l = l->prev) {
    GType type = 0;

    pad_template = (GstPadTemplate *) l->data;
    EDITOR_LOG ("evaluating padtemplate %s\n", GST_OBJECT_NAME (pad_template));

    switch (pad_template->presence) {
      case GST_PAD_SOMETIMES:
        type = gst_editor_pad_sometimes_get_type ();
        break;
      case GST_PAD_REQUEST:
        type = gst_editor_pad_request_get_type ();
        break;
      case GST_PAD_ALWAYS:
        EDITOR_LOG
            ("Error in element %s: ALWAYS pad template '%s', but no pad provided",
            g_type_name (G_OBJECT_TYPE (e)), GST_OBJECT_NAME (pad_template));
        continue;
    }

    editor_pad =
        //        GST_EDITOR_ITEM (gnome_canvas_item_new (GNOME_CANVAS_GROUP (element),
        //            type, "object", G_OBJECT (pad_template), NULL));
        GST_EDITOR_ITEM (goo_canvas_item_new (GOO_CANVAS_ITEM (element),
            type, "object", G_OBJECT (pad_template), NULL));
    gst_editor_pad_realize (editor_pad);


    if (GST_PAD_TEMPLATE_DIRECTION (pad_template) == GST_PAD_SINK) {
      element->sinkpads = g_list_prepend (element->sinkpads, editor_pad);
      element->sinks++;
    } else {
      element->srcpads = g_list_prepend (element->srcpads, editor_pad);
      element->srcs++;
    }
  }
  //gst_editor_element_move(element,0.0,0.0);
  /* the caller must repack the element (we do it by moving it about 0)*/
}

static void
gst_editor_element_remove_pad (GstEditorElement * element, GstPad * pad)
{
  GstEditorItem *editor_pad;

  editor_pad = gst_editor_item_get (GST_OBJECT (pad));

  if (GST_PAD_DIRECTION (pad) == GST_PAD_SINK) {
    element->sinkpads = g_list_remove (element->sinkpads, editor_pad);
    element->sinks--;
  } else {
    element->srcpads = g_list_remove (element->srcpads, editor_pad);
    element->srcs--;
  }
}

static void
gst_editor_element_set_state (GstEditorElement * element, GstState state)
{
  GstEditorItem *item = GST_EDITOR_ITEM (element);

  if (item->object)
    gst_element_set_state (GST_ELEMENT (item->object), state);
}

static gboolean
gst_editor_element_set_state_cb (GstEditorElement * element)
{
  //g_print("gst_editor_element_set_state_cb\n");
  if (element->next_state != GST_STATE_VOID_PENDING)
    gst_editor_element_set_state (element, element->next_state);

  element->next_state = GST_STATE_VOID_PENDING;
  element->set_state_idle_id = 0;
  return FALSE;
}

/* return a bool so we can be a GSourceFunc */
/*static*/ gboolean
gst_editor_element_sync_state (GstEditorElement * element)
{
   if (element==NULL){
    g_print("Fixme: gst_editor_element_sync_state called with element=NULL!\n");
    return FALSE;
    }
   if (!GST_IS_EDITOR_ELEMENT(element)){
    g_print("Fixme: gst_editor_element_sync_state called with element that is no element!\n");
    return FALSE;
    }
  g_static_rw_lock_writer_lock (GST_EDITOR_ITEM(element)->globallock);
  //g_print("Sync State!\n");
  gint id;
  GstEditorItem *item;
  GstState state;
  gdouble x1, x2, y1, y2;

  item = GST_EDITOR_ITEM (element);

  if (item->object == NULL){
    g_print("Warning: Sync State Item Object Invalid!\n");
    g_static_rw_lock_writer_unlock (GST_EDITOR_ITEM(element)->globallock);
    return FALSE;
  }
  state = GST_STATE (GST_ELEMENT (item->object));

  /* make sure args to gnome_canvas_item_set are doubles */
  x1 = 0.0;
  y1 = 0.0;
  x2 = item->width;
  y2 = item->height;

  for (id = 0; id < 4; id++) {
    if (_gst_element_states[id] == state) {
//      gnome_canvas_item_set (element->statebox,
//          "x1", x1 + (element->statewidth * id),
//          "y1", y2 - element->stateheight,
//          "x2", x1 + (element->statewidth * (id + 1)), "y2", y2, NULL);
      g_object_set (element->statebox,
          "x", x1 + (element->statewidth * id),
          "y", y2 - element->stateheight,
          "width", element->statewidth, "height", element->stateheight, NULL);
    }
  }
  g_static_rw_lock_writer_unlock (GST_EDITOR_ITEM(element)->globallock);
  return FALSE;
}

/**********************************************************************
 * Popup menu calbacks
 **********************************************************************/

static void
on_copy (GtkWidget * unused, GstEditorElement * element)
{
  g_return_if_fail (GST_IS_EDITOR_ELEMENT (element));

  gst_editor_element_copy (element);
}

static void
on_cut (GtkWidget * unused, GstEditorElement * element)
{
  g_return_if_fail (GST_IS_EDITOR_ELEMENT (element));

  gst_editor_element_cut (element);
}

static void
on_remove (GtkWidget * unused, GstEditorElement * element)
{
  g_return_if_fail (GST_IS_EDITOR_ELEMENT (element));

  gst_editor_element_remove (element);
}

/**********************************************************************
 * Public functions
 **********************************************************************/

void
gst_editor_element_move (GstEditorElement * element, gdouble dx, gdouble dy)
{
  GstEditorItem *parent;
  GooCanvasBounds bounds, parentbounds;
  gdouble x, y, h, w;

  /* we must stay within the parent bin, if any */
  parent =
      GST_EDITOR_ITEM (goo_canvas_item_get_parent (GOO_CANVAS_ITEM (element)));
  if (GST_IS_EDITOR_BIN (parent)) {
    gdouble top, bottom, left, right;

    top = parent->t.h;
    bottom = parent->b.h;
    left = parent->l.w;
    right = parent->r.w;

    goo_canvas_item_get_bounds (GOO_CANVAS_ITEM (parent), &parentbounds);

    /* element x and y are with respect to the upper-left corner of the bin */
    goo_canvas_item_get_bounds (GOO_CANVAS_ITEM (element), &bounds);
    x = bounds.x1;
    y = bounds.y1;
    w = bounds.x2 - bounds.x1;
    h = bounds.y2 - bounds.y1;
    //g_print("Element bounds: x1 %f x2 %f y1 %f y2 %f w %f h %f\n", x,bounds.x2,bounds.y1,bounds.y2,w,h);
	//for testing only
    //gdouble x2,y2,scale,rotation;
    //struct cairo_matrix_t* matrix;
    //cairo_matrix_t matrix;
    //cairo_matrix_init_identity(matrix); 
    //goo_canvas_item_get_simple_transform(GOO_CANVAS_ITEM (element),&x2,&y2,&scale,&rotation); 
    //g_print("Element transform: x %f y %f Scale %f Rotation %f And saved width %f %f\n",x2,y2,scale,rotation,GST_EDITOR_ITEM(element)->width,GST_EDITOR_ITEM(element)->height );
    //goo_canvas_item_get_transform(GOO_CANVAS_ITEM (element),&matrix);
    //g_print("Element transform: x %f y %f saved width %f %f\n",matrix.x0,matrix.y0,GST_EDITOR_ITEM(element)->width,GST_EDITOR_ITEM(element)->height );


    if (parent->height - top - bottom < h || parent->width - left - right < w) {
      g_warning ("bin is too small");
      return;
    }

/*  if (x + dx < left) {
      dx = left - x;
    } else if (x + dx + w > parent->width - right) {
      dx = parent->width - right - w - x;
    }

    if (y + dy < 0) {
       dy = 0;
    }
    else if (y + dy < top) {
      dy = top - y;
    } else if (y + dy + h > parent->height - bottom) {
      dy = parent->height - bottom - h - y;
    }*/

    if (x + dx < 0) {
      //g_print("Gotten stucked in x: %f and would like to do dx %f \n", x,dx);
      dx = 0;
    } else if (x + dx < parentbounds.x1 + left) {
      //g_print("Approaching left band x: %f ; dx %f , Parentbounds.x1 %f , leftbar of parent %f \n", x,dx,parentbounds.x1,left);
      dx = parentbounds.x1 + left - x;
    } else if (x + dx + w > parentbounds.x2 - right) {
      //g_print("Approaching right band x: %f ; dx %f , Parentbounds.x2 %f , rightbar of parent %f \n", x,dx,parentbounds.x2,right);
      dx = parentbounds.x2 - right - w - x;
    }

    if (y + dy < 0) {
      dy = 0;
    } else if (y + dy < parentbounds.y1 + top) {
      dy = parentbounds.y1 + top - y;
    } else if (y + dy + h > parentbounds.y2 - bottom) {
      dy = parentbounds.y2 - bottom - h - y;
    }
  }

  gst_editor_item_move (GST_EDITOR_ITEM (element), dx, dy);
  //g_print("finished moving element %p\n",element);
  //usleep(1);
}

void
gst_editor_element_cut (GstEditorElement * element)
{
  gst_editor_element_copy (element);
  gst_editor_element_remove (element);
}

void
gst_editor_element_copy (GstEditorElement * element)
{
  xmlChar *mem;
  gint size = 0;
  GtkClipboard *clipboard;
  GstElement *e;

  xmlIndentTreeOutput = 1;

  e = GST_ELEMENT (GST_EDITOR_ITEM (element)->object);

  xmlDocDumpFormatMemory (gst_xml_write (e), &mem, &size, 1);

  if (!size) {
    g_warning ("copy failed");
    return;
  }

  clipboard = gtk_clipboard_get (GDK_NONE);
  /* should this be text/xml ? */
  gtk_clipboard_set_text (clipboard, (gchar *) mem, size);
}

void
gst_editor_element_remove (GstEditorElement * element)
{
  GstBin *bin;
  GstElement *e;

  e = GST_ELEMENT (GST_EDITOR_ITEM (element)->object);
  bin = (GstBin *) GST_OBJECT_PARENT (e);

  if (!bin) {
    g_object_set (goo_canvas_item_get_canvas (GOO_CANVAS_ITEM (element)),
        "status",
        "Could not remove element: Removal of toplevel bin is not allowed.",
        NULL);

    return;
  }

  gsth_element_unlink_all (e);
  gst_bin_remove (bin, e);
}

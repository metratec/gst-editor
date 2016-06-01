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

#include <gst/editor/editor.h>
#include <gst/common/gste-marshal.h>
#include <gst/common/gste-debug.h>

#include "gst-helper.h"
    GST_DEBUG_CATEGORY (gste_item_debug);
#define GST_CAT_DEFAULT gste_item_debug

/* interface methods */ 
static void canvas_item_interface_init (GooCanvasItemIface * iface);
static gboolean gst_editor_item_button_press_event (GooCanvasItem * citem,
    GooCanvasItem * target_item, GdkEventButton * event);
static gboolean gst_editor_element_button_release_event (GooCanvasItem * citem,
    GooCanvasItem * target, GdkEventButton * event);

/* class methods */
static void gst_editor_item_class_init (GstEditorItemClass * klass);
static void gst_editor_item_init (GstEditorItem * item);
static void gst_editor_item_dispose (GObject * object);
static void gst_editor_item_finalize (GObject * object);
static void gst_editor_item_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_editor_item_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);


/*static*/ void gst_editor_item_realize (GooCanvasItemSimple * citem);
//void gst_editor_item_realize (GooCanvasItem * citem);
static void gst_editor_item_object_changed (GstEditorItem * item,
    GstObject * object);
static void gst_editor_item_resize_real (GstEditorItem * item);
static void gst_editor_item_repack_real (GstEditorItem * item);
static void gst_editor_item_default_on_whats_this (GstEditorItem * item);


/* callbacks on the parent item */
static void on_parent_item_position_changed (GstEditorItem * parent,
    GstEditorItem * item);

/* callbacks on the editor item */
static void on_object_saved (GstObject * object, xmlNodePtr parent,
    GstEditorItem * item);



/* popup callbacks */
/*static*/ void on_whats_this (GtkWidget * unused, GstEditorItem * item);


enum
{
  ARG_0, ARG_WIDTH, ARG_HEIGHT,
  ARG_OBJECT,
ARG_GLOBALLOCK
};

enum
{
  OBJECT_CHANGED,
  POSITION_CHANGED,
  LAST_SIGNAL
};

static GObjectClass *parent_class;
static guint gst_editor_item_signals[LAST_SIGNAL] = { 0 };
static GHashTable *editor_items = NULL;


#ifdef POPUP_MENU
static GnomeUIInfo menu_items[] = {
  GNOMEUIINFO_ITEM_STOCK ("What's this?",
      "Give information on the currently selected item",
      on_whats_this, "gtk-dialog-info"),
  GNOMEUIINFO_END
};
#endif  /*  */
static const GtkActionEntry action_entries[] = { 
      {"whats_this", "gtk-dialog-info", "What's this?", NULL,
        "Give information on the currently selected item",
        G_CALLBACK (on_whats_this)}, 
};
static const char *ui_description = "<ui>" 
    "  <popup name='itemMenu'>" 
    "    <menuitem name='whats_this' position='bot' action='whats_this'  />" 
    "  </popup>"  "</ui>";

/* helper functions */
    static void
gst_editor_item_update_title (GstEditorItem * item)
{
  if (item->title_text)
    g_free (item->title_text);
  item->title_text =
      g_strdup (item->object ? GST_OBJECT_NAME (item->object) : "");
  if ((item->title)&&(item->object)){
    g_object_set (G_OBJECT (item->title), "text", item->title_text, NULL);
    GST_DEBUG ("updated title of editor item to %s", item->title_text);
    }
    else GST_DEBUG ("did not updated title because editor element seems to be deleted");
}

static void
gst_editor_notify_name_cb (GstObject * object, GParamSpec * arg1,
    GstEditorItem * item)
{
  GST_DEBUG ("name changed on GstObject %s", GST_OBJECT_NAME (object));

  g_return_if_fail (GST_IS_EDITOR_ITEM (item));

  /* FIXME: we could do a block/unblock by_func, see mr project */
  gst_editor_item_update_title (item);
}

/* GOBject-y stuff */
GType
gst_editor_item_get_type (void)
{
  static GType item_type = 0;

  if (item_type == 0) {
    static const GTypeInfo item_info = {
      sizeof (GstEditorItemClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gst_editor_item_class_init,
      NULL,
      NULL,
      sizeof (GstEditorItem),
      0,
      (GInstanceInitFunc) gst_editor_item_init,
    };
    static const GInterfaceInfo iitem_info = { 
          (GInterfaceInitFunc) canvas_item_interface_init, /* interface_init */
          
          NULL, /* interface_finalize */ 
          NULL /* interface_data */  
    };
    item_type =
        g_type_register_static (goo_canvas_group_get_type (), "GstEditorItem",
        &item_info, 0);
    g_type_add_interface_static (item_type, GOO_TYPE_CANVAS_ITEM,
        &iitem_info);
  }
  return item_type;
}

static void
gst_editor_item_class_init (GstEditorItemClass * klass)
{
  GObjectClass *object_class;

  GError * error = NULL;

  object_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_ref (goo_canvas_group_get_type ());

  gst_editor_item_signals[OBJECT_CHANGED] =
      g_signal_new ("object-changed", G_OBJECT_CLASS_TYPE (klass),
      G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GstEditorItemClass, object_changed),
      NULL, NULL, gst_editor_marshal_VOID__OBJECT, G_TYPE_NONE, 1,
      GST_TYPE_OBJECT);
  gst_editor_item_signals[POSITION_CHANGED] =
      g_signal_new ("position-changed", G_OBJECT_CLASS_TYPE (klass),
      G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GstEditorItemClass,
          position_changed), NULL, NULL, gst_editor_marshal_VOID__VOID,
      G_TYPE_NONE, 0);

  object_class->set_property = gst_editor_item_set_property;
  object_class->get_property = gst_editor_item_get_property;
  object_class->dispose = gst_editor_item_dispose;
  object_class->finalize = gst_editor_item_finalize;
  g_object_class_install_property (object_class, ARG_WIDTH,
      g_param_spec_double ("width", "width", "width",
          0, G_MAXDOUBLE, 30, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, ARG_HEIGHT,
      g_param_spec_double ("height", "height", "height",
          0, G_MAXDOUBLE, 10, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, ARG_OBJECT,
      g_param_spec_object ("object", "object", "object",
          GST_TYPE_OBJECT, G_PARAM_READWRITE));
    g_object_class_install_property (object_class, ARG_GLOBALLOCK,
      g_param_spec_pointer ("globallock", "globallock", "globallock",
          G_PARAM_WRITABLE));
#if 0
      citem_class->realize = gst_editor_item_realize;
  
#endif
      klass->repack = gst_editor_item_repack_real;
  klass->resize = gst_editor_item_resize_real;
  klass->object_changed = gst_editor_item_object_changed;
  klass->whats_this = gst_editor_item_default_on_whats_this;
  
#ifdef POPUP_MENU
      GST_EDITOR_ITEM_CLASS_PREPEND_MENU_ITEMS (klass, menu_items, 1);
#endif  /*  */
  GST_EDITOR_ITEM_CLASS_PREPEND_ACTION_ENTRIES (klass, action_entries, 1);
  klass->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_add_ui_from_string (klass->ui_manager, ui_description, -1,
      &error);
  GST_DEBUG_CATEGORY_INIT (gste_item_debug, "GSTE_ITEM", 0,
      "GStreamer Editor Item");
}
static void 
canvas_item_interface_init (GooCanvasItemIface * iface) 
{
  iface->button_press_event = gst_editor_item_button_press_event;
} static void

gst_editor_item_init (GstEditorItem * item)
{
  item->fill_color = 0xffffffff;
  item->outline_color = 0x333333ff;
  item->title_text = g_strdup ("rename me");
  item->textx = 1.0;
  item->texty = 1.0;
  item->height = 10;
  item->width = 30;
  item->textanchor = GTK_ANCHOR_NORTH_WEST;
} static void

gst_editor_item_finalize (GObject * object)
{
  EDITOR_LOG ("Finalizing GstEditorItem 0x%p\n", object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_editor_item_dispose (GObject * object)
{
  GstEditorItem *item = GST_EDITOR_ITEM (object);

  EDITOR_LOG ("Disposing GstEditorItem %p\n", object);

  if (item->object) {
    if (GST_IS_ELEMENT(item->object)) g_print("Disposing GstEditorItem with object %s\n", GST_OBJECT_NAME(item->object));
    gst_editor_item_hash_remove(item->object);
    g_object_set (item, "object", NULL, NULL);
    item->object = NULL;
  }
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_editor_item_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstEditorItem *item = GST_EDITOR_ITEM (object);

  switch (prop_id) {
    case ARG_WIDTH:
      item->width = g_value_get_double (value);
      break;
    case ARG_HEIGHT:
      item->height = g_value_get_double (value);
      break;
    case ARG_OBJECT:
    {
      GstObject *new_object = GST_OBJECT (g_value_get_object (value));

      /* Check if the object actually changes before setting off */
      if (new_object != item->object) {
        GObject *old_object;

        if (new_object)
          g_object_ref (G_OBJECT (new_object));

        g_signal_emit (object, gst_editor_item_signals[OBJECT_CHANGED], 0,
            new_object, NULL);
        /* remove old notify handler */
        if (item->notify_cb_id) {
          g_signal_handler_disconnect (item->object, item->notify_cb_id);
          item->notify_cb_id = 0;
        }

        old_object = G_OBJECT (item->object);
        item->object = new_object;

        if (old_object)
          g_object_unref (G_OBJECT (old_object));

        /* install new notify handler */
        if (new_object) {
          item->notify_cb_id = g_signal_connect (item->object, "notify::name",
              G_CALLBACK (gst_editor_notify_name_cb), item);
        }
      }
    }
      gst_editor_item_update_title (item);
      break;
    case ARG_GLOBALLOCK:
      item->globallock = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  if ((item->realized)&&(item->object))
    gst_editor_item_resize (item);
}

static void
gst_editor_item_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstEditorItem *item = GST_EDITOR_ITEM (object);

  switch (prop_id) {
    case ARG_WIDTH:
      g_value_set_double (value, item->width);
      break;
    case ARG_HEIGHT:
      g_value_set_double (value, item->height);
      break;
    case ARG_OBJECT:
      g_value_set_object (value, G_OBJECT (item->object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/*static*/ void 
gst_editor_item_realize (GooCanvasItemSimple * citem)
//gst_editor_item_realize (GooCanvasItem * citem)
{
  GstEditorItem *item = GST_EDITOR_ITEM (citem);

  
#if 0
      if (GNOME_CANVAS_ITEM_CLASS (parent_class)->realize)
    GNOME_CANVAS_ITEM_CLASS (parent_class)->realize (citem);
#endif  /*  */
  item->border =
      goo_canvas_rect_new (GOO_CANVAS_ITEM (citem), 0., 0., 0., 0.,
      "line-width", 1., "fill-color-rgba", item->fill_color,
      "stroke-color-rgba", item->outline_color, "antialias",
      CAIRO_ANTIALIAS_NONE, NULL);
  
//  goo_canvas_item_lower_pos (item->border, 10);
if (!GOO_IS_CANVAS_ITEM(item->border)){
g_print("item border corrupted!!!\n");
sleep(10);
}
      goo_canvas_item_lower (item->border, NULL);
  g_return_if_fail (item->border != NULL);
  GST_EDITOR_SET_OBJECT (item->border, item);
  item->title =
      goo_canvas_text_new (GOO_CANVAS_ITEM (citem), NULL, 0, 0, -1,
      GTK_ANCHOR_NW, "font", "Sans", "fill-color", "black", NULL);
  g_return_if_fail (item->title != NULL);
  g_object_set (G_OBJECT (item->title), "text", item->title_text, NULL);
  GST_EDITOR_SET_OBJECT (item->title, item);

  item->realized = TRUE;

  /* emission of position-changed on a parent item will propogate to all
     children */
  if (GST_IS_EDITOR_ITEM (citem->parent))
    g_signal_connect (citem->parent, "position-changed",
        G_CALLBACK (on_parent_item_position_changed), citem);

  if (G_OBJECT_TYPE (item) == GST_TYPE_EDITOR_ITEM)
    gst_editor_item_resize (item);
}

static void
gst_editor_item_resize_real (GstEditorItem * item)
{
  if (!GST_IS_EDITOR_ITEM(item)){
	g_print("gst_editor_item_resize_real: item %p  corrupted!!!\n",item);
	return;
  }
  gdouble itemwidth, itemheight;

  GooCanvasBounds bounds;

  /* it won't have a title prior to being realized */
  if (item->title) {
//    g_object_get (G_OBJECT (item->title), "text-width", &itemwidth, NULL);
//    g_object_get (G_OBJECT (item->title), "text-height", &itemheight, NULL);
    if (!GOO_IS_CANVAS_ITEM(item->title)){
    g_print("gst_editor_item_resize_real: item title corrupted %p !!!\n",item);
    return;
    }
    goo_canvas_item_get_bounds (item->title, &bounds);
    itemwidth = bounds.x2 - bounds.x1;
    itemheight = bounds.y2 - bounds.y1;
    item->t.w += itemwidth + 2.0;
    item->t.h = MAX (item->t.h, itemheight + 2.0);
  }

  /* force the thing to grow if necessary */
//  item->width = MAX (MAX (MAX (item->t.w, item->b.w),
//          item->l.w + item->c.w + item->r.w), item->width);
//  item->height =
//      MAX (MAX (MAX (item->l.h, item->c.h), item->r.h) + item->t.h + item->b.h,
//      item->height);
  item->width =
      MAX (MAX (MAX (item->t.w, item->b.w), item->l.w + item->r.w),
      item->width);
  item->height =
      MAX (MAX (item->l.h, item->r.h) + item->t.h + item->b.h, item->height);
//g_print("nearly finished resize %p\n",item);
  GST_EDITOR_ITEM_CLASS (G_OBJECT_GET_CLASS (item))->repack (item);
//g_print("finished resize\n");
//  g_signal_emit (G_OBJECT (item), gst_editor_item_signals[SIZE_CHANGED], 0, NULL);
}

static void
gst_editor_item_repack_real (GstEditorItem * item)
{
//  gdouble x1, y1, x2, y2;

  if (!item->realized)
    return;

//  x1 = 0;
//  y1 = 0;
//  x2 = x1 + item->width;
//  y2 = y1 + item->height;

  /* resize the bordering box */
//  gnome_canvas_item_set (item->border,
//      "x1", x1, "y1", y1, "x2", x2, "y2", y2, NULL);
  g_object_set (G_OBJECT (item->border), "x", 0., "y", 0., "width",
      item->width, "height", item->height, NULL);

  /* move the text to the right place */ 
      g_object_set (G_OBJECT (item->title), "x", item->textx, "y", item->texty,
      "anchor", item->textanchor, NULL);
}

static void
gst_editor_item_object_changed (GstEditorItem * item, GstObject * object)
{
  if (!editor_items)
    editor_items = g_hash_table_new (NULL, NULL);

  /* doesn't handle removals yet */
  g_hash_table_insert (editor_items, object, item);

  if (item->object) {
    g_signal_handlers_disconnect_by_func (G_OBJECT (item->object),
        on_object_saved, item);
  }

  if (object) {
    g_signal_connect (G_OBJECT (object), "object-saved",
        G_CALLBACK (on_object_saved), item);
  }
}


#if 0
static gint
gst_editor_item_event (GnomeCanvasItem * citem, GdkEvent * event) 
{
  GstEditorItem *item;

  item = GST_EDITOR_ITEM (citem);

  switch (event->type) {
    case GDK_BUTTON_PRESS:
      if (event->button.button == 3) {
        GstEditorItemClass *itemclass =
            GST_EDITOR_ITEM_CLASS (G_OBJECT_GET_CLASS (citem));

        /* although we don't actually show a menu, stop the event propagation */
        if (itemclass->menu_items == 0)
          return TRUE;

        if (!itemclass->menu)
          itemclass->menu = gnome_popup_menu_new (itemclass->menu_items);

        gnome_popup_menu_do_popup (itemclass->menu, NULL, NULL, &event->button,
            item, NULL);
      }
      break;
    default:
      break;
  }

  /* we don't want the click falling through to the parent canvas item */
  return TRUE;
}
#endif  /*  */
static gint 
gst_editor_item_button_press_event (GooCanvasItem * citem,
    GooCanvasItem * target_item, GdkEventButton * event) 
{
  GstEditorItem * item;
  item = GST_EDITOR_ITEM (citem);
  if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
    GstEditorItemClass * itemclass =
        GST_EDITOR_ITEM_CLASS (G_OBJECT_GET_CLASS (citem));
    
#ifdef POPUP_MENU
        /* although we don't actually show a menu, stop the event propagation */ 
        if (itemclass->menu_items == 0)
      return TRUE;
    if (!itemclass->menu)
      itemclass->menu = gnome_popup_menu_new (itemclass->menu_items);
    gnome_popup_menu_do_popup (itemclass->menu, NULL, NULL, &event->button,
        item, NULL);
    
#endif  /*  */
        /* although we don't actually show a menu, stop the event propagation */ 
        if (itemclass->num_action_entries == 0)
      return TRUE;
     {
      GtkActionGroup * action_group;
      action_group = gtk_action_group_new ("PopupActions");
      gtk_action_group_add_actions (action_group, itemclass->action_entries,
          itemclass->num_action_entries, item);
      gtk_ui_manager_insert_action_group (itemclass->ui_manager, action_group,
          0);
      g_object_unref (G_OBJECT (action_group));
      if (!itemclass->menu)
        itemclass->menu =
            gtk_ui_manager_get_widget (itemclass->ui_manager, "/itemMenu");
      gtk_menu_popup (GTK_MENU (itemclass->menu), NULL, NULL, NULL, NULL,
          (event ? event->button : 0),
          (event ? event->time : gtk_get_current_event_time ()));
    }
    
        // return FALSE;
  }
  
      /* we don't want the click falling through to the parent canvas item */ 
      return TRUE;
}
static void
gst_editor_item_default_on_whats_this (GstEditorItem * item)
{
  GtkWidget *dialog;

  dialog =
      gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
      "Items of type %s have not implemented a what's this page, sorry.",
      g_type_name (G_OBJECT_TYPE (item)));

  g_signal_connect_swapped (G_OBJECT (dialog), "response",
      G_CALLBACK (gtk_widget_destroy), G_OBJECT (dialog));

  gtk_widget_show (dialog);
}


/**********************************************************************
 * Callbacks on the editor item
 **********************************************************************/
static void
on_object_saved (GstObject * object, xmlNodePtr parent, GstEditorItem * item)
{
  GooCanvasBounds bounds;
  xmlNsPtr ns;
  xmlNodePtr child;
  gchar *value;
  gdouble x, y, width, height;

  /* first see if the namespace is already known */
  ns = xmlSearchNsByHref (parent->doc, parent,
      "http://gstreamer.net/gst-editor/1.0/");
  if (ns == NULL) {
    xmlNodePtr root = xmlDocGetRootElement (parent->doc);

    /* add namespace to root node */
    ns = xmlNewNs (root, "http://gstreamer.net/gst-editor/1.0/", "gst-editor");
  }

  child = xmlNewChild (parent, ns, "item", NULL);
//  g_object_get (G_OBJECT (item), "x", &x, "y", &y,
//      "width", &width, "height", &height, NULL);
/* goo_canvas_item_get_bounds (GOO_CANVAS_ITEM (item), &bounds);
  x = bounds.x1;
  y = bounds.y1;
  width = bounds.x2 - bounds.x1;
  height = bounds.y2 - bounds.y1;
*/
//this does not work, since links are included in the total size...
cairo_matrix_t matrix;
goo_canvas_item_get_transform(GOO_CANVAS_ITEM (item),&matrix);
x=matrix.x0;
y=matrix.y0;
g_object_get (G_OBJECT (item),"width", &width, "height", &height, NULL);GST_DEBUG_OBJECT (object, "saving with position x: %f, y: %f, %fx%f",
      GST_OBJECT_NAME (object), x, y, width, height);
  value = g_strdup_printf ("%f", x);
  xmlNewChild (child, ns, "x", value);
  g_free (value);
  value = g_strdup_printf ("%f", y);
  xmlNewChild (child, ns, "y", value);
  g_free (value);
  value = g_strdup_printf ("%f", width);
  xmlNewChild (child, ns, "w", value);
  g_free (value);
  value = g_strdup_printf ("%f", height);
  xmlNewChild (child, ns, "h", value);
  g_free (value);
}

/**********************************************************************
 * Callbacks on the parent editor item
 **********************************************************************/

static void
on_parent_item_position_changed (GstEditorItem * parent, GstEditorItem * item)
{
  g_return_if_fail (item != parent);

  g_signal_emit (G_OBJECT (item), gst_editor_item_signals[POSITION_CHANGED], 0,
      NULL);
}

/**********************************************************************
 * Popup menu callbacks
 **********************************************************************/

/*static*/ void
on_whats_this (GtkWidget * unused, GstEditorItem * item)
{
  GstEditorItemClass *klass = GST_EDITOR_ITEM_CLASS (G_OBJECT_GET_CLASS (item));

  g_return_if_fail (klass->whats_this != NULL);

  klass->whats_this (item);
}

/**********************************************************************
 * Public functions
 **********************************************************************/

void
gst_editor_item_repack (GstEditorItem * item)
{
  if (GST_EDITOR_ITEM_CLASS (G_OBJECT_GET_CLASS (item))->repack)
    (GST_EDITOR_ITEM_CLASS (G_OBJECT_GET_CLASS (item))->repack) (item);
}

void
gst_editor_item_resize (GstEditorItem * item)
{
  GstEditorItemBand empty = { 0.0, 0.0 };

  item->l = empty;
  item->r = empty;
  item->t = empty;
  item->b = empty;
//  item->c = empty;

  if (GST_EDITOR_ITEM_CLASS (G_OBJECT_GET_CLASS (item))->resize)
    (GST_EDITOR_ITEM_CLASS (G_OBJECT_GET_CLASS (item))->resize) (item);
}

GstEditorItem *
gst_editor_item_get (GstObject * object)
{
  GstEditorItem *ret;

  if (!editor_items)
    ret = NULL;
  else
    ret = (GstEditorItem *) g_hash_table_lookup (editor_items, object);

  return ret;
}

void
gst_editor_item_disconnect (GstEditorItem * parent,GstEditorItem * child)
{

  g_signal_handlers_disconnect_by_func (G_OBJECT (parent),
        on_parent_item_position_changed, child);
  return;
}

void
gst_editor_item_hash_remove (GstObject * object)
{
  if (editor_items) g_hash_table_remove (editor_items, object);
  return;
}

void
gst_editor_item_move (GstEditorItem * item, gdouble dx, gdouble dy)
{
  g_return_if_fail (GST_IS_EDITOR_ITEM (item));
if (!item->object) g_print("Warning, item: %p has no object\n",item);  goo_canvas_item_translate (GOO_CANVAS_ITEM (item), dx, dy);
//g_print("translate finished, emitting signal\n");  
  g_signal_emit ((GObject *) item, gst_editor_item_signals[POSITION_CHANGED], 0,
      item);
//g_print("Signal emitted\n");  
}

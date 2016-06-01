#include "editor.h"
#include "gst-helper.h"
#include <gst/common/gste-debug.h>

/* class functions */
static void gst_editor_link_class_init (GstEditorLinkClass * klass);
static void gst_editor_link_init (GstEditorLink * link);

static void gst_editor_link_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_editor_link_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
/*static*/ void gst_editor_link_realize (GooCanvasItem * citem);
static void gst_editor_link_resize (GstEditorLink * link);

/* callbacks from gstreamer */
static void on_pad_unlink (GstPad * pad, GstPad * peer, GstEditorLink * link);
static void on_new_pad (GstElement * element, GstPad * pad,
    GstEditorLink * link);

/* callbacks from editor pads */
static void on_editor_pad_position_changed (GstEditorPad * pad,
    GstEditorLink * link);

/* utility */
static void make_dynamic_link (GstEditorLink * link);


enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2,
  PROP_SRCPAD,
  PROP_SINKPAD,
  PROP_GHOST,
};

enum
{
  LAST_SIGNAL
};

static GObjectClass *parent_class;


/* static guint gst_editor_link_signals[LAST_SIGNAL] = { 0 }; */
    GType
gst_editor_link_get_type (void)
{
  static GType link_type = 0;

  if (link_type == 0) {
    static const GTypeInfo link_info = {
      sizeof (GstEditorLinkClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gst_editor_link_class_init,
      NULL,
      NULL,
      sizeof (GstEditorLink),
      0,
      (GInstanceInitFunc) gst_editor_link_init,
    };

    link_type =
        g_type_register_static (goo_canvas_polyline_get_type (),
        "GstEditorLink", &link_info, 0);
  }
  return link_type;
}

static void
gst_editor_link_class_init (GstEditorLinkClass * klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_ref (goo_canvas_polyline_get_type ());

  object_class->set_property = gst_editor_link_set_property;
  object_class->get_property = gst_editor_link_get_property;

  g_object_class_install_property (object_class, PROP_X,
      g_param_spec_double ("x", "x", "x",
          -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_Y,
      g_param_spec_double ("y", "y", "y",
          -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_X1,
      g_param_spec_double ("x1", "x1", "x1",
          -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_Y1,
      g_param_spec_double ("y1", "y1", "y1",
          -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_X2,
      g_param_spec_double ("x2", "x2", "x2",
          -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_Y2,
      g_param_spec_double ("y2", "y2", "y2",
          -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_SRCPAD,
      g_param_spec_object ("src-pad", "src-pad", "src-pad",
          gst_editor_pad_get_type (), G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SINKPAD,
      g_param_spec_object ("sink-pad", "sink-pad", "sink-pad",
          gst_editor_pad_get_type (), G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_GHOST,
      g_param_spec_boolean ("ghost", "ghost", "ghost",
          FALSE, G_PARAM_READWRITE));
  
#if 0
      GNOME_CANVAS_ITEM_CLASS (klass)->realize = gst_editor_link_realize;
  
#endif
}

static void
gst_editor_link_init (GstEditorLink * link)
{
  link->points = goo_canvas_points_new (2);
  
//   goo_canvas_item_raise_pos ((GooCanvasItem *) link, 10);
      goo_canvas_item_raise ((GooCanvasItem *) link, NULL);
}

static void
gst_editor_link_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstEditorPad *srcpad, *sinkpad;
  GstEditorLink *link = GST_EDITOR_LINK (object);

  switch (prop_id) {
    case PROP_X:
      if (link->srcpad && link->sinkpad)
        g_warning ("Settting link drag x without having one unset pad");

      link->dragging = TRUE;
      link->x = g_value_get_double (value);
      break;

    case PROP_Y:
      if (link->srcpad && link->sinkpad)
        g_warning ("Settting link drag y without having one unset pad");

      link->dragging = TRUE;
      link->y = g_value_get_double (value);
      break;

    case PROP_SRCPAD:
      srcpad = (GstEditorPad *) link->srcpad;
      sinkpad = (GstEditorPad *) link->sinkpad;

      if (srcpad) {
        if (link->ghost)
          srcpad->ghostlink = NULL;
        else
          srcpad->link = NULL;
      }

      srcpad = (GstEditorPad *) g_value_get_object (value);

      if (srcpad) {
        if (link->ghost)
          srcpad->ghostlink = link;
        else
          srcpad->link = link;

        if (sinkpad)
          link->dragging = FALSE;
      }

      link->srcpad = (GstEditorItem *) srcpad;
      break;

    case PROP_SINKPAD:
      srcpad = (GstEditorPad *) link->srcpad;
      sinkpad = (GstEditorPad *) link->sinkpad;

      if (sinkpad) {
        if (link->ghost)
          sinkpad->ghostlink = NULL;
        else
          sinkpad->link = NULL;
      }

      sinkpad = (GstEditorPad *) g_value_get_object (value);

      if (sinkpad) {
        if (link->ghost)
          sinkpad->ghostlink = link;
        else
          sinkpad->link = link;

        if (srcpad)
          link->dragging = FALSE;
      }

      link->sinkpad = (GstEditorItem *) sinkpad;
      break;

    case PROP_GHOST:
      link->ghost = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  /* check if we're ready to resize */
  if (((link->srcpad || link->sinkpad) && link->dragging) ||
      (link->srcpad && link->sinkpad))
    gst_editor_link_resize (link);
}

static void
gst_editor_link_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GooCanvas * canvas;
  if ((object==NULL)||(!GST_IS_EDITOR_LINK(object))){
    g_print("Warning: gst_editor_link_realize failed because citem %p is no gst_editor_link\n",object);
    return;
  }
  GstEditorLink *link = GST_EDITOR_LINK (object);
  gdouble d = 0.0, blah = 0.0;

  switch (prop_id) {
    case PROP_X:
      g_value_set_double (value, link->x);
      break;

    case PROP_Y:
      g_value_set_double (value, link->y);
      break;

    case PROP_X1:
      if (link->srcpad) {
        
// FIXME
//         g_object_get (link->srcpad, "x", &d, NULL);
//         d += link->srcpad->width;
//         gnome_canvas_item_i2w (GNOME_CANVAS_ITEM (link->srcpad)->parent, &d,
//                  &blah);
            d = link->srcpad->width;
        canvas = goo_canvas_item_get_canvas (GOO_CANVAS_ITEM (link->srcpad));
        goo_canvas_convert_from_item_space (canvas,
            GOO_CANVAS_ITEM (link->srcpad), &d, &blah);
        if (canvas) goo_canvas_convert_to_pixels (canvas, &d, &blah);
else g_print("fixme: Canvas Item (X2, srcpad) got no Canvas:%p, Pixels d: %f, blah: %f\n",canvas, d,blah);
      } else if (link->dragging) {
        d = link->x;
      } else {
        g_warning ("no src pad");
      }
      g_value_set_double (value, d);
      break;

    case PROP_X2:
      if (link->sinkpad) {
// FIXME
//         g_object_get (link->sinkpad, "x", &d, NULL);
//         gnome_canvas_item_i2w (GNOME_CANVAS_ITEM (link->sinkpad)->parent, &d,
//          &blah);
        d = 0;
        canvas = goo_canvas_item_get_canvas (GOO_CANVAS_ITEM (link->sinkpad));
//g_print("Canvas:%p, Pixels d: %f, blah: %f\n",canvas, d,blah); 
      goo_canvas_convert_from_item_space (canvas,
            GOO_CANVAS_ITEM (link->sinkpad), &d, &blah);
//FIXME: Segfaults here
//g_print("Canvas:%p, Pixels d: %f, blah: %f\n",canvas, d,blah);
        if (canvas) goo_canvas_convert_to_pixels (canvas, &d, &blah);
else {
  g_print("fixme: Canvas Item (X2, sinkpad) got no Canvas:%p, Pixels d: %f, blah: %f\n",canvas, d,blah);
  d=d+5;
}
//g_print("Canvas:%p, Pixels d: %f, blah: %f\n",canvas, d,blah);
      } else if (link->dragging) {
        d = link->x;
      } else {
        g_warning ("no sink pad");
      }

      g_value_set_double (value, d);
      break;

    case PROP_Y1:
      if (link->srcpad) {
// FIXME
//            g_object_get (link->srcpad, "y", &d, NULL);
//       d += link->srcpad->height / 2;
//            gnome_canvas_item_i2w (GNOME_CANVAS_ITEM (link->srcpad)->parent, &blah,
//                &d);

        d = link->srcpad->height / 2;
        canvas = goo_canvas_item_get_canvas (GOO_CANVAS_ITEM (link->srcpad));
        goo_canvas_convert_from_item_space (canvas,
            GOO_CANVAS_ITEM (link->srcpad), &blah, &d);
        if (canvas) goo_canvas_convert_to_pixels (canvas, &blah, &d);
else g_print("fixme: Canvas Item (Y1, srcpad) got no Canvas:%p, Pixels d: %f, blah: %f\n",canvas, d,blah);
      } else if (link->dragging) {
        d = link->y;
      } else {
        g_warning ("no src pad");
      }
      g_value_set_double (value, d);
      break;

    case PROP_Y2:
      if (link->sinkpad) {
// FIXME
//            g_object_get (link->sinkpad, "y", &d, NULL);
//            d += link->sinkpad->height / 2;
//            gnome_canvas_item_i2w (GNOME_CANVAS_ITEM (link->sinkpad)->parent, &blah,
//               &d);
        d = link->sinkpad->height / 2;
        canvas = goo_canvas_item_get_canvas (GOO_CANVAS_ITEM (link->sinkpad));
        goo_canvas_convert_from_item_space (canvas,
            GOO_CANVAS_ITEM (link->sinkpad), &blah, &d);
if (canvas) goo_canvas_convert_to_pixels (canvas, &blah, &d);
else {
  g_print("fixme: Canvas Item (Y2, sinkpad) got no Canvas:%p, Pixels d: %f, blah: %f\n",canvas, d,blah);
  d=d+5;
}        
      } else if (link->dragging) {
        d = link->y;
      } else {
        g_warning ("no sink pad");
      }
      g_value_set_double (value, d);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


/*static*/ void
gst_editor_link_realize (GooCanvasItem * citem)
{
  GooCanvasLineDash * dash;
  if ((citem==NULL)||(!GST_EDITOR_LINK(citem))){
    g_print("Warning: gst_editor_link_realize failed because citem %p is no gst_editor_link",citem);
    return;
  }
  GstEditorLink *link = GST_EDITOR_LINK (citem);

  link->points->coords[0] = 0.0;
  link->points->coords[1] = 0.0;
  link->points->coords[2] = 0.0;
  link->points->coords[3] = 0.0;

  /* we need to be realized before setting properties */ 
#if 0
      if (GNOME_CANVAS_ITEM_CLASS (parent_class)->realize)
    GNOME_CANVAS_ITEM_CLASS (parent_class)->realize (citem);
#endif  /*  */
  dash = goo_canvas_line_dash_new (2, 5.0, 5.0);
  
      /* see gnome-canvas-line.h for the docs */ 
      g_object_set (G_OBJECT (citem), "points", link->points, "line-width",
      2.0, "line-dash", dash, "start-arrow", TRUE, NULL);
  goo_canvas_line_dash_unref (dash);
  
//  goo_canvas_item_raise_pos (citem, 10);
      goo_canvas_item_raise (citem, NULL);
}

static void
gst_editor_link_resize (GstEditorLink * link)
{
  gdouble x1, y1, x2, y2;

  GooCanvas * canvas;

  g_object_get (link, "x1", &x1, "y1", &y1, "x2", &x2, "y2", &y2, NULL);
//  gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (link)->parent, &x1, &y1);
//  gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (link)->parent, &x2, &y2);
  canvas = goo_canvas_item_get_canvas (GOO_CANVAS_ITEM (link));
if (!canvas) {
g_print("Warning: gst_editor_link_resize canvas null\n");
return;
}goo_canvas_convert_from_pixels (canvas, &x1, &y1);
  goo_canvas_convert_to_item_space (canvas, GOO_CANVAS_ITEM (link), &x1, &y1);
  
      //goo_canvas_item_get_parent(GOO_CANVAS_ITEM (link)),&x1, &y1);
      goo_canvas_convert_from_pixels (canvas, &x2, &y2);
  goo_canvas_convert_to_item_space (canvas, GOO_CANVAS_ITEM (link), &x2, &y2);
  
      //goo_canvas_item_get_parent(GOO_CANVAS_ITEM (link)), &x2, &y2);
      /* we do this in reverse so that with dot-dash lines it gives the illusion of
         pulling out a rope from the element */
      link->points->coords[2] = x1;
  link->points->coords[3] = y1;
  link->points->coords[0] = x2;
  link->points->coords[1] = y2;
  g_object_set (G_OBJECT (link), "points", link->points, NULL);
}

static void
on_pad_unlink (GstPad * pad, GstPad * peer, GstEditorLink * link)
{
  GStaticRWLock* globallock;
  if ((GST_IS_EDITOR_LINK(link))&&(GST_IS_EDITOR_PAD(link->srcpad))) globallock=GST_EDITOR_ITEM(link->srcpad)->globallock;
  else if ((GST_IS_EDITOR_LINK(link))&&(GST_IS_EDITOR_PAD(link->sinkpad))) globallock=GST_EDITOR_ITEM(link->sinkpad)->globallock;
  else {g_print("Warning: On pad unlink without any valid Pads called!!!!");
  return;
  }
  g_static_rw_lock_writer_lock (globallock);
        g_print("Unlink pad signal (%s:%s from %s:%s) with link %p",
        GST_DEBUG_PAD_NAME (pad), GST_DEBUG_PAD_NAME (peer), link);
  GstEditorBin *srcbin=NULL, *sinkbin=NULL;

  /* this function is called when unlinking dynamic links as well */
  if (peer && pad)
    GST_CAT_DEBUG (gste_debug_cat,
        "Unlink pad signal (%s:%s from %s:%s) with link %p",
        GST_DEBUG_PAD_NAME (pad), GST_DEBUG_PAD_NAME (peer), link);
  else
    GST_CAT_DEBUG (gste_debug_cat, "Unlinking dynamic link");
  if (!GST_IS_EDITOR_PAD(link->srcpad))g_print( "Warning: Unlink pad signals critical: Srcpad not valid\n");
  if (link->srcpad) g_signal_handlers_disconnect_by_func (link->srcpad,
      on_editor_pad_position_changed, link);
  if (!GST_IS_EDITOR_PAD(link->sinkpad))g_print( "Warning: Unlink pad signals critical: Sinkpad not valid\n");
  if (link->sinkpad) g_signal_handlers_disconnect_by_func (link->sinkpad,
      on_editor_pad_position_changed, link);
  if (GST_IS_PAD (pad))g_signal_handlers_disconnect_by_func (pad, on_pad_unlink, link);
  else g_print( "Warning: On_pad_unlink: Pad not valid\n");
  if (GST_IS_PAD (peer))g_signal_handlers_disconnect_by_func (peer, on_pad_unlink, link);
  else g_print( "Warning: On_pad_unlink: Peer not valid\n");
  g_print("Unlinked  pad signals, get srcbin\n");
  if (link->srcpad) srcbin =
      GST_EDITOR_BIN (goo_canvas_item_get_parent (goo_canvas_item_get_parent
          (GOO_CANVAS_ITEM (link->srcpad))));
  g_print("Unlinked  pad signals, get sinkbin\n");  if (link->sinkpad) sinkbin =
      GST_EDITOR_BIN (goo_canvas_item_get_parent (goo_canvas_item_get_parent
          (GOO_CANVAS_ITEM (link->sinkpad))));
g_print("Removing links from bin; srcbin: %p, sinkbin %p\n",srcbin,sinkbin);    if (sinkbin) sinkbin->links = g_list_remove (sinkbin->links, link);
  else g_print( "Warning: On_pad_unlink: Sinkbin not valid\n");
  if ((sinkbin != srcbin)&&(srcbin)) srcbin->links = g_list_remove (srcbin->links, link);
  else g_print( "Warning: On_pad_unlink: Srcbin not validor same as Sinkbin\n");
  if (link->ghost){
    if (link->srcpad) GST_EDITOR_PAD (link->srcpad)->ghostlink = NULL;
    if (link->sinkpad)GST_EDITOR_PAD (link->sinkpad)->ghostlink = NULL;
  }
  else{
    if (link->srcpad) GST_EDITOR_PAD (link->srcpad)->link = NULL;
    if (link->sinkpad)GST_EDITOR_PAD (link->sinkpad)->link = NULL;
  }
  g_static_rw_lock_writer_unlock (globallock);
  link->srcpad = NULL;
  link->sinkpad = NULL;
  /* i have bad luck with actually killing the GCI's */ 
      //goo_canvas_item_simple_hide (GOO_CANVAS_ITEM_SIMPLE (link));

      if (GOO_IS_CANVAS_ITEM(link)) goo_canvas_item_remove(GOO_CANVAS_ITEM(link));
  g_print("Finished on_pad_unlink\n");

}

static void
on_editor_pad_position_changed (GstEditorPad * pad, GstEditorLink * link)
{
  //g_print("On editor pad position changed\n");
  g_return_if_fail (GST_IS_EDITOR_LINK (link));

  gst_editor_link_resize (link);
}

gboolean
gst_editor_link_link (GstEditorLink * link)
{
  GObject *src, *sink;
  GstPad *srcpad = NULL, *sinkpad = NULL;

  GooCanvasItem * item;
  GooCanvasLineDash * dash;

  g_return_val_if_fail (GST_IS_EDITOR_LINK (link), FALSE);

  if (!link->srcpad || !link->sinkpad)
    goto error;

  src = (GObject *) GST_EDITOR_ITEM (link->srcpad)->object;
  sink = (GObject *) GST_EDITOR_ITEM (link->sinkpad)->object;
  if (!GST_EDITOR_PAD (link->srcpad)->istemplate)
     {
    if (!GST_EDITOR_PAD (link->sinkpad)->istemplate)
       {
      if (GST_PAD_PEER (src) || GST_PAD_PEER (sink))
         {
        /*if (GST_PAD_PEER (src) != (GstPad *) sink) 
           {
           g_warning ("The src pad is linked, but not to the sink pad");
           goto error;
           }
           if (GST_PAD_PEER (sink) != (GstPad *) src) 
           {
           g_warning ("The sink pad is linked, but not to the src pad");
           goto error;
           } */
        srcpad = GST_PAD (src);
        sinkpad = GST_PAD (sink);
        goto linked;
        /* yay goto */
        }
      }
    
    else if (GST_PAD_PEER (src))
       {
      /* the to pad is a template */
      g_warning ("The src pad is linked, but not to the sink pad");
      goto error;
      }
    }
  
  else if (!GST_EDITOR_PAD (link->sinkpad)->istemplate && GST_PAD_PEER (sink))
     {
    /* from pad is a template */
    g_warning ("The sink pad is linked, but not to the src pad");
    goto error;
    }

  if (link->ghost)
     {
    g_object_set (G_OBJECT (GOO_CANVAS_ITEM (link)), "fill-color", "grey70",
        NULL);
    goto linked;
    }
  
  else if (GST_IS_EDITOR_PAD_SOMETIMES (link->srcpad) ||
      GST_IS_EDITOR_PAD_SOMETIMES (link->sinkpad))
     {
    make_dynamic_link (link);
    g_object_set (G_OBJECT (GOO_CANVAS_ITEM (link)), "fill-color", "grey50",
        NULL);
    goto linked;
    }
  
  else
     {
    if (!GST_EDITOR_PAD (link->srcpad)->istemplate)
       {
      srcpad = GST_PAD (src);
      }
    
    else if (GST_IS_EDITOR_PAD_REQUEST (link->srcpad))
       {
      srcpad = gst_element_get_request_pad ((GstElement *) 
          GST_EDITOR_ITEM (goo_canvas_item_get_parent (GOO_CANVAS_ITEM (link->
                      srcpad)))->object,
          GST_PAD_TEMPLATE (src)->name_template);
      /* the new_pad signal will cause a new pad to made automagically in the
         element */ 
          g_object_set (G_OBJECT (GOO_CANVAS_ITEM (link)), "src-pad",
          gst_editor_item_get ((GstObject *) srcpad), NULL);
      }
    
    else
       {
      goto error;
      }

    if (!srcpad)
      goto error;

    if (!GST_EDITOR_PAD (link->sinkpad)->istemplate)
       {
      sinkpad = GST_PAD (sink);
      }
    
    else if (GST_IS_EDITOR_PAD_REQUEST (link->sinkpad))
       {
      sinkpad = gst_element_get_request_pad ((GstElement *) 
          GST_EDITOR_ITEM (goo_canvas_item_get_parent (GOO_CANVAS_ITEM (link->
                      sinkpad)))->object,
          GST_PAD_TEMPLATE (sink)->name_template);
      g_object_set (G_OBJECT (GOO_CANVAS_ITEM (link)), "sink-pad",
          gst_editor_item_get ((GstObject *) sinkpad), NULL);
      }
    
    else
       {
      goto error;
      }

    if (!sinkpad)
      goto error;

    if (gst_pad_link (srcpad, sinkpad) == GST_PAD_LINK_OK)
       {
      GstEditorBin *srcbin, *sinkbin;

      g_object_set (G_OBJECT (GOO_CANVAS_ITEM (link)), "fill-color", "black",
          NULL);

    linked:

      srcbin = sinkbin = NULL;
      dash = goo_canvas_line_dash_new (0, NULL);
      g_object_set (G_OBJECT (GOO_CANVAS_ITEM (link)), "line-dash", dash,
          NULL);
      goo_canvas_line_dash_unref (dash);

      g_signal_connect (link->srcpad, "position-changed",
          G_CALLBACK (on_editor_pad_position_changed), G_OBJECT (link));
      g_signal_connect (link->sinkpad, "position-changed",
          G_CALLBACK (on_editor_pad_position_changed), G_OBJECT (link));

      /* don't connect a signal on a ghost or dynamic link */
      g_print("adding unlinked signal for src: %p and sink %p",srcpad,sinkpad);
      if (srcpad && sinkpad)
         {
        GST_CAT_DEBUG (gste_debug_cat,
            "link pad signal (%s:%s from %s:%s) with link %p",
            GST_DEBUG_PAD_NAME (srcpad), GST_DEBUG_PAD_NAME (sinkpad), link);
        g_signal_connect (srcpad, "unlinked", G_CALLBACK (on_pad_unlink), link);
        g_signal_connect (sinkpad, "unlinked", G_CALLBACK (on_pad_unlink), link);
        }
      item =
          goo_canvas_item_get_parent (goo_canvas_item_get_parent
          (GOO_CANVAS_ITEM (link->srcpad)));
      if (GST_IS_EDITOR_BIN (item))
        srcbin = GST_EDITOR_BIN (item);

      item =
          goo_canvas_item_get_parent (goo_canvas_item_get_parent
          (GOO_CANVAS_ITEM (link->sinkpad)));
      if (GST_IS_EDITOR_BIN (item))
        sinkbin = GST_EDITOR_BIN (item);
      if (sinkbin)
        sinkbin->links = g_list_prepend (sinkbin->links, link);
      if (srcbin && sinkbin != srcbin)
        srcbin->links = g_list_prepend (srcbin->links, link);

      return TRUE;
      }
    }

error:
  g_message ("could not link");

  if (link->srcpad)
    GST_EDITOR_PAD (link->srcpad)->link = NULL;
  if (link->sinkpad)
    GST_EDITOR_PAD (link->sinkpad)->link = NULL;
  return FALSE;
}

void
gst_editor_link_destroy(GstEditorLink * link){
 //g_print ("trying to destroy this half link");
 GstPad *pad;
 GstEditorBin *padbin;
 if (link->srcpad)
      padbin =
      GST_EDITOR_BIN (goo_canvas_item_get_parent (	goo_canvas_item_get_parent
          (GOO_CANVAS_ITEM (link->srcpad))));
 else padbin =
      GST_EDITOR_BIN (goo_canvas_item_get_parent (	goo_canvas_item_get_parent
          (GOO_CANVAS_ITEM (link->sinkpad)))); 
  padbin->links = g_list_remove (padbin->links, link);
  if (link->srcpad) GST_EDITOR_PAD(link->srcpad)->link=NULL;
  if (link->sinkpad) GST_EDITOR_PAD(link->sinkpad)->link=NULL;	
  int killnumber;
  //killnumber=goo_canvas_item_find_child(goo_canvas_item_get_parent(GOO_CANVAS_ITEM(link)),GOO_CANVAS_ITEM(link));
  //g_print ("found child at %d",killnumber); 
  //ARGH, what am I doing here
  //g_realloc(link,sizeof(GooCanvasPolyline));

  
  //g_realloc(link,sizeof(GooCanvasPolyline));
  //goo_canvas_item_remove_child(goo_canvas_item_get_parent(GOO_CANVAS_ITEM(link)),killnumber);
  goo_canvas_item_remove(GOO_CANVAS_ITEM(link));
  if (link->points) goo_canvas_points_unref(link->points);
 //g_print ("ready to crash:)");  
 //g_free(link);
 //g_print ("no warnings until now"); 
 return;
}
void
gst_editor_link_unlink (GstEditorLink * link)
{
  if(link->srcpad) GST_EDITOR_PAD (link->srcpad)->unlinking = FALSE;
  if(link->sinkpad) GST_EDITOR_PAD (link->sinkpad)->unlinking = FALSE;

  if (link->ghost) {
    g_warning ("this function should not be called for ghost links..");
  } else if (!GST_EDITOR_PAD (link->srcpad)->istemplate &&
      !GST_EDITOR_PAD (link->sinkpad)->istemplate) {
    GstPad *src = NULL, *sink = NULL;

    g_object_get (link->srcpad, "object", &src, NULL);
    g_object_get (link->sinkpad, "object", &sink, NULL);

    gst_pad_unlink (src, sink);

    /* 'unlinked' signal takes care of calling on_pad_unlink */
  } else {
    /* we must have a dynamic link, then.. */

    g_signal_handlers_disconnect_by_func (link->srcpad->object,
        on_new_pad, link);
    g_signal_handlers_disconnect_by_func (link->sinkpad->object,
        on_new_pad, link);

    on_pad_unlink (NULL, NULL, link);
  }

  /* potential threadsafety issues here, can't assume that the signal has
   * already been handled. you might not even be able to assume you can
   * unlink the pads. maybe the pipeline should only be modifiable in
   * PAUSED? */

  /* but then again calling gst_pad_unlink on a PLAYING threaded element
     isn't safe anyway. i guess we're ok.. */

  /* 'link' is now invalid */
}

/* this function is only linked for dynamic links */
static void
on_new_pad (GstElement * element, GstPad * pad, GstEditorLink * link)
{
  GstPadTemplate *src = NULL, *sink = NULL;

  if (GST_IS_EDITOR_PAD_SOMETIMES (link->srcpad))
    src = GST_PAD_TEMPLATE (link->srcpad->object);

  if (GST_IS_EDITOR_PAD_SOMETIMES (link->sinkpad))
    sink = GST_PAD_TEMPLATE (link->sinkpad->object);

  g_message ("new pad");
  if (pad->padtemplate) {
    g_message ("from a template");

    /* can't do pointer comparison -- some templates appear to be from the
       elementfactories, some from template factories... have to compare
       names */
    if (src
        && g_ascii_strcasecmp (pad->padtemplate->name_template,
            src->name_template) == 0)
      g_object_set (G_OBJECT (GOO_CANVAS_ITEM (link)), "src-pad",
          gst_editor_item_get (GST_OBJECT (pad)), NULL);
    else if (sink
        && g_ascii_strcasecmp (pad->padtemplate->name_template,
            sink->name_template) == 0)
      g_object_set (G_OBJECT (GOO_CANVAS_ITEM (link)), "sink-pad",
          gst_editor_item_get (GST_OBJECT (pad)), NULL);
    else
      return;

    g_message ("we made it, now let's link");

    /*gst_element_set_state ((GstElement *)
       gst_element_get_managing_bin (element), GST_STATE_PAUSED); */
    gst_editor_link_link (link);
    /*gst_element_set_state ((GstElement *)
       gst_element_get_managing_bin (element), GST_STATE_PLAYING); */
  }
}

static void
make_dynamic_link (GstEditorLink * link)
{
  GstElement *srce, *sinke;
  GstPadTemplate *src = NULL, *sink = NULL;

  if (GST_IS_EDITOR_PAD_SOMETIMES (link->srcpad))
    src = GST_PAD_TEMPLATE (link->srcpad->object);

  if (GST_IS_EDITOR_PAD_SOMETIMES (link->sinkpad))
    sink = GST_PAD_TEMPLATE (link->sinkpad->object);
  srce =
      GST_ELEMENT (GST_EDITOR_ITEM (goo_canvas_item_get_parent (GOO_CANVAS_ITEM
              (link->srcpad)))->object);
  sinke =
      GST_ELEMENT (GST_EDITOR_ITEM (goo_canvas_item_get_parent (GOO_CANVAS_ITEM
              (link->sinkpad)))->object);

  g_return_if_fail (src || sink);

  if (src)
    g_signal_connect_after (srce, "pad-added", G_CALLBACK (on_new_pad), link);
  if (sink)
    g_signal_connect_after (sinke, "pad-added", G_CALLBACK (on_new_pad), link);

  g_print ("dynamic link\n");
}

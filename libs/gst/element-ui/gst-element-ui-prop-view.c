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

#include <string.h>
#include "gst-element-ui-prop-view.h"

GST_DEBUG_CATEGORY (gste_element_ui_debug);
#define GST_CAT_DEFAULT gste_element_ui_debug

static void gst_element_ui_prop_view_class_init (GstElementUIPropViewClass *
    klass);
static void gst_element_ui_prop_view_init (GstElementUIPropView * pview);
static void gst_element_ui_prop_view_dispose (GObject * object);

static void gst_element_ui_prop_view_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_element_ui_prop_view_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

static void pview_param_changed (GstElementUIPropView * pview);
static void pview_value_changed (GstElementUIPropView * pview);
static void on_adjustment_value_changed (GtkAdjustment * adjustment,
    GstElementUIPropView * pview);
static void on_toggle_button_toggled (GtkToggleButton * button,
    GstElementUIPropView * pview);
static void on_entry_activate (GtkEntry * entry, GstElementUIPropView * pview);
static void on_optionmenu_changed (GtkOptionMenu * option,
    GstElementUIPropView * pview);
static void on_location_hit (GtkWidget * widget, GstElementUIPropView * pview);
static void block_signals (GstElementUIPropView * pview);
static void unblock_signals (GstElementUIPropView * pview);

static GObjectClass *parent_class = NULL;

enum
{
  PROP_0,
  PROP_ELEMENT,
  PROP_PARAM
};

GType
gst_element_ui_prop_view_get_type (void)
{
  static GType element_ui_prop_view_type = 0;

  if (!element_ui_prop_view_type) {
    static const GTypeInfo element_ui_prop_view_info = {
      sizeof (GstElementUIPropViewClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_element_ui_prop_view_class_init,
      NULL,
      NULL,
      sizeof (GstElementUIPropView),
      0,
      (GInstanceInitFunc) gst_element_ui_prop_view_init,
    };

    element_ui_prop_view_type =
        g_type_register_static (GTK_TYPE_VBOX, "GstElementUIPropView",
        &element_ui_prop_view_info, 0);
  }
  return element_ui_prop_view_type;
}

static void
gst_element_ui_prop_view_class_init (GstElementUIPropViewClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gst_element_ui_prop_view_dispose;

  parent_class = (GObjectClass *) g_type_class_ref (gtk_vbox_get_type ());

  object_class->set_property = gst_element_ui_prop_view_set_property;
  object_class->get_property = gst_element_ui_prop_view_get_property;

  g_object_class_install_property (object_class, PROP_ELEMENT,
      g_param_spec_object ("element", "Element", "Element",
          gst_element_get_type (), G_PARAM_READWRITE));

  /* anyone know the right way to do this? */
  g_object_class_install_property (object_class, PROP_PARAM,
      g_param_spec_pointer ("param", "Param",
          "The GParamSpec to view and control", G_PARAM_READWRITE));

  GST_DEBUG_CATEGORY_INIT (gste_element_ui_debug, "GSTE_ELEMENT_UI", 0,
      "GStreamer Editor Element UI");
}

static void
gst_element_ui_prop_view_init (GstElementUIPropView * pview)
{
  GtkWidget *table_args;
  GtkWidget *table_spin;
  GtkWidget *label_lower;
  GtkObject *spinbutton_adj;
  GtkWidget *spinbutton;
  GtkWidget *toggle_on;
  GtkWidget *toggle_off;
  GtkWidget *entry;
  GtkWidget *label_upper;
  GtkWidget *hscale;
  GtkWidget *optionmenu;
  GtkWidget *file;
  GtkWidget *filetext;

  pview->value = g_new0 (GValue, 1);
  pview->value_mutex = g_mutex_new ();

  table_args = gtk_table_new (6, 6, FALSE);
  gtk_widget_show (table_args);
  gtk_box_pack_start (GTK_BOX (pview), table_args, FALSE, TRUE, 0);

  table_spin = gtk_table_new (1, 3, TRUE);
  gtk_widget_show (table_spin);
  gtk_table_attach (GTK_TABLE (table_args), table_spin, 0, 6, 0, 1,
      GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  label_lower = gtk_label_new ("");
  gtk_table_attach (GTK_TABLE (table_spin), label_lower, 0, 1, 0, 1, GTK_FILL,
      GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (label_lower), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label_lower), 0, 0.5);

  spinbutton_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton_adj), 1, 0);
  gtk_table_attach (GTK_TABLE (table_spin), spinbutton, 1, 2, 0, 1,
      GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  label_upper = gtk_label_new ("");
  gtk_table_attach (GTK_TABLE (table_spin), label_upper, 2, 3, 0, 1, GTK_FILL,
      GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (label_upper), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label_upper), 1, 0.5);

  toggle_on = gtk_toggle_button_new_with_label ("on");
  gtk_table_attach (GTK_TABLE (table_args), toggle_on, 0, 3, 1, 2,
      GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  toggle_off = gtk_toggle_button_new_with_label ("off");
  gtk_table_attach (GTK_TABLE (table_args), toggle_off, 3, 6, 1, 2,
      GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table_args), entry, 0, 6, 2, 3,
      GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  hscale = gtk_hscale_new (GTK_ADJUSTMENT (spinbutton_adj));
  gtk_table_attach (GTK_TABLE (table_args), hscale, 0, 6, 3, 4,
      GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_scale_set_draw_value (GTK_SCALE (hscale), FALSE);
  gtk_scale_set_digits (GTK_SCALE (hscale), 2);

  optionmenu = gtk_option_menu_new ();
  gtk_table_attach (GTK_TABLE (table_args), optionmenu, 0, 6, 4, 5,
      GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);


  file = gtk_button_new_from_stock (GTK_STOCK_OPEN);;
  gtk_table_attach (GTK_TABLE (table_args), file, 0, 1, 5, 6,
      GTK_SHRINK, GTK_FILL, 0, 0);
  filetext = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table_args), filetext, 1, 6, 5, 6,
      GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);


  g_signal_connect (spinbutton_adj, "value_changed",
      G_CALLBACK (on_adjustment_value_changed), pview);
  g_signal_connect (toggle_off, "toggled",
      G_CALLBACK (on_toggle_button_toggled), pview);
  g_signal_connect (toggle_on, "toggled", G_CALLBACK (on_toggle_button_toggled),
      pview);
  g_signal_connect (entry, "activate", G_CALLBACK (on_entry_activate), pview);
  g_signal_connect (filetext, "activate", G_CALLBACK (on_entry_activate), pview);
  g_signal_connect (optionmenu, "changed", G_CALLBACK (on_optionmenu_changed),
      pview);
  g_signal_connect (file, "clicked", G_CALLBACK (on_location_hit),
      pview);

  pview->adjustment = spinbutton_adj;
  pview->label_lower = label_lower;
  pview->spinbutton = spinbutton;
  pview->toggle_on = toggle_on;
  pview->toggle_off = toggle_off;
  pview->entry = entry;
  pview->label_upper = label_upper;
  pview->hscale = hscale;
  pview->optionmenu = optionmenu;
  pview->file = file;
  pview->filetext = filetext;
}

static void
gst_element_ui_prop_view_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstElementUIPropView *pview;

  pview = GST_ELEMENT_UI_PROP_VIEW (object);

  switch (prop_id) {
    case PROP_ELEMENT:
      pview->element = GST_ELEMENT (g_value_get_object (value));
      if (pview->param)
        g_object_set (object, "param", NULL, NULL);
      break;
    case PROP_PARAM:
      if (!pview->element) {
        g_warning
            ("\"element\" must be set before \"param\" for GstElementUIPropView instances");
        return;
      }

      pview->param = G_PARAM_SPEC (g_value_get_pointer (value));

      g_mutex_lock (pview->value_mutex);

      /* G_IS_VALUE (zeroed or unset value) == FALSE */
      if (G_IS_VALUE (pview->value))
        g_value_unset (pview->value);

      g_value_init (pview->value, pview->param->value_type);
      g_object_get_property (G_OBJECT (pview->element), pview->param->name,
          pview->value);
      g_mutex_unlock (pview->value_mutex);

      if (!(pview->param->flags & G_PARAM_WRITABLE))
        g_object_set (pview, "sensitive", FALSE, NULL);
      else
        g_object_set (pview, "sensitive", TRUE, NULL);

      pview_param_changed (pview);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }
}

static void
gst_element_ui_prop_view_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstElementUIPropView *pview;

  pview = GST_ELEMENT_UI_PROP_VIEW (object);

  switch (prop_id) {
    case PROP_ELEMENT:
      g_value_set_object (value, G_OBJECT (pview->element));
      break;
    case PROP_PARAM:
      g_value_set_pointer (value, pview->param);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_element_ui_prop_view_dispose (GObject * object)
{
  GstElementUIPropView *pview;

  pview = GST_ELEMENT_UI_PROP_VIEW (object);

  if (pview->source_id)
    g_source_remove (pview->source_id);
  pview->source_id = 0;

  if (G_OBJECT_CLASS (parent_class)->dispose)
    (*G_OBJECT_CLASS (parent_class)->dispose) (object);
}

/* It is assumed that this function is called from the element's thread, so we
   can just go ahead and use g_object_get here */
gboolean
gst_element_ui_prop_view_update_async (GstElementUIPropView * pview)
{
  GST_DEBUG ("async property update: %s", pview->param->name);

  g_mutex_lock (pview->value_mutex);

  g_object_get_property ((GObject *) pview->element, pview->param->name,
      pview->value);

  /* we have to have a mutex around the pview->source_id variable, and it might
   * as well be here */

  GST_DEBUG ("source_id: %ld", (glong) pview->source_id);

  if (!pview->source_id) {
    pview->source_id = g_timeout_add (0,
        (GSourceFunc) gst_element_ui_prop_view_update, pview);
    GST_DEBUG ("adding timeout for property update with id %d",
        pview->source_id);
  }

  g_mutex_unlock (pview->value_mutex);

  return FALSE;
}

gboolean
gst_element_ui_prop_view_update (GstElementUIPropView * pview)
{
  block_signals (pview);

  g_mutex_lock (pview->value_mutex);
  gchar *contents;
  
  GST_DEBUG ("Name of parameters to update view %s\n",G_PARAM_SPEC_TYPE_NAME(pview->param));
  
  contents = g_strdup_value_contents (pview->value);
  GST_DEBUG ("updating prop view to new value %s", contents);
  g_free (contents);


  /* clear the update request flag, within the value_mutex */
  GST_DEBUG ("resetting pview->source_id to 0");
  pview->source_id = 0;

  switch (pview->param->value_type) {
    case G_TYPE_BOOLEAN:
      pview->on_set = pview->on_pending = g_value_get_boolean (pview->value);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pview->toggle_on),
          pview->on_set);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pview->toggle_off),
          !pview->on_set);
      break;
    case G_TYPE_INT:
      gtk_adjustment_set_value (GTK_ADJUSTMENT (pview->adjustment),
          g_value_get_int (pview->value));
      break;
    case G_TYPE_UINT:
      gtk_adjustment_set_value (GTK_ADJUSTMENT (pview->adjustment),
          g_value_get_uint (pview->value));
      break;
    case G_TYPE_LONG:
      gtk_adjustment_set_value (GTK_ADJUSTMENT (pview->adjustment),
          g_value_get_long (pview->value));
      break;
    case G_TYPE_ULONG:
      gtk_adjustment_set_value (GTK_ADJUSTMENT (pview->adjustment),
          g_value_get_ulong (pview->value));
      break;
    case G_TYPE_INT64:
      gtk_adjustment_set_value (GTK_ADJUSTMENT (pview->adjustment),
          g_value_get_int64 (pview->value));
      break;
#ifndef _MSC_VER
    case G_TYPE_UINT64:
      gtk_adjustment_set_value (GTK_ADJUSTMENT (pview->adjustment),
          g_value_get_uint64 (pview->value));
      break;
      
#else   /*  */
#pragma message ("Check if cast from uint64 to double is supported with msvc 6.0")
#endif
    case G_TYPE_FLOAT:
      gtk_adjustment_set_value (GTK_ADJUSTMENT (pview->adjustment),
          g_value_get_float (pview->value));
      break;
    case G_TYPE_DOUBLE:
      gtk_adjustment_set_value (GTK_ADJUSTMENT (pview->adjustment),
          g_value_get_double (pview->value));
      break;
    case G_TYPE_STRING:
      gtk_entry_set_text (GTK_ENTRY (pview->entry),
          g_value_get_string (pview->value) ? g_value_get_string (pview->
              value) : "");
      gtk_entry_set_text (GTK_ENTRY (pview->filetext),
          g_value_get_string (pview->value) ? g_value_get_string (pview->
              value) : "");
      break;
    default:
      if (G_IS_PARAM_SPEC_ENUM (pview->param)) {
        gint i = 0, val = g_value_get_enum (pview->value);

        while (pview->enum_values[i] != val)
          i++;
        gtk_option_menu_set_history (GTK_OPTION_MENU (pview->optionmenu), i);
      } 
      else if (!strcmp("GstCaps",g_type_name (pview->param->value_type))){
        //g_print("Try to show caps\n");
	GstCaps * temp=gst_value_get_caps(pview->value);
        if(temp){
          char* capsstring=NULL;
          capsstring=gst_caps_to_string(temp);
          gtk_entry_set_text (GTK_ENTRY (pview->entry),capsstring);
          g_free(capsstring);
        }
	else gtk_entry_set_text (GTK_ENTRY (pview->entry),"Caps are NULL");
      }
      else if (!strcmp("GstPad",g_type_name (pview->param->value_type))){
	gint i = 0;
	GstPad* val = (GstPad*)g_value_get_object (pview->value);
	while (pview->enum_pointer[i] != val) {
		//g_print("Keep on searching for %p,Round %d\n",val,i);
		i++;
	}
        gtk_option_menu_set_history (GTK_OPTION_MENU (pview->optionmenu), i);
      }
      else {
        g_warning ("prop_view_update for type %s not yet implemented",
            g_type_name (pview->param->value_type));
      }
  }

  g_mutex_unlock (pview->value_mutex);
  unblock_signals (pview);
  GST_DEBUG ("property updated");
  return FALSE;
}

static void
pview_param_changed (GstElementUIPropView * pview)
{
  GstElement *element = pview->element;
  GParamSpec *param = pview->param;
  GtkAdjustment *adj;
  gchar *lower, *upper;
  const gchar *format;

  block_signals (pview);

  GST_DEBUG ("::param changed, reconstructing prop view");

  if (!element || !param)
    return;

  GST_DEBUG ("(name = %s)", param->name);

  gtk_widget_hide (pview->entry);
  gtk_widget_hide (pview->toggle_on);
  gtk_widget_hide (pview->toggle_off);
  gtk_widget_hide (pview->spinbutton);
  gtk_widget_hide (pview->label_lower);
  gtk_widget_hide (pview->label_upper);
  gtk_widget_hide (pview->hscale);
  gtk_widget_hide (pview->optionmenu);
  gtk_widget_hide (pview->file);
  gtk_widget_hide (pview->filetext);

  switch (param->value_type) {
    case G_TYPE_BOOLEAN:
      gtk_widget_show (pview->toggle_on);
      gtk_widget_show (pview->toggle_off);
      break;

#define CASE_NUMERIC(type, digits)                                              \
    case G_TYPE_##type:                                                         \
        gtk_widget_show(pview->spinbutton);                                     \
        gtk_widget_show(pview->hscale);                                         \
        gtk_widget_show(pview->label_lower);                                    \
        gtk_widget_show(pview->label_upper);                                    \
                                                                                \
        adj = GTK_ADJUSTMENT (pview->adjustment);                               \
        adj->lower = G_PARAM_SPEC_##type (param)->minimum;                      \
        adj->upper = G_PARAM_SPEC_##type (param)->maximum;                      \
        adj->value = (adj->upper - adj->lower) / 2;                             \
        adj->step_increment = (adj->upper - adj->lower) / 20;                   \
        adj->page_increment = (adj->upper - adj->lower) / 4;                    \
        adj->page_size = 0;                                                     \
        gtk_adjustment_changed (adj);                                           \
                                                                                \
        if (digits == 0 &&                                                      \
            (ABS(adj->lower) > 100000 || ABS(adj->upper) > 100000))             \
            format = "%.4g";                                                    \
        else if (digits == 0)                                                   \
            format = "%.0f";                                                    \
        else                                                                    \
            format = "%."#digits"g";                                            \
        lower = g_strdup_printf (format, adj->lower);                           \
        upper = g_strdup_printf (format, adj->upper);                           \
        gtk_label_set_text(GTK_LABEL(pview->label_lower), lower);               \
        gtk_label_set_text(GTK_LABEL(pview->label_upper), upper);               \
        g_free (lower);                                                         \
        g_free (upper);                                                         \
                                                                                \
        gtk_scale_set_digits(GTK_SCALE(pview->hscale), digits);                 \
        gtk_spin_button_set_digits(GTK_SPIN_BUTTON(pview->spinbutton), digits); \
        break

      CASE_NUMERIC (FLOAT, 2);
      CASE_NUMERIC (DOUBLE, 2);
      CASE_NUMERIC (INT, 0);
      CASE_NUMERIC (UINT, 0);
      CASE_NUMERIC (LONG, 0);
      CASE_NUMERIC (ULONG, 0);
      
#ifndef WIN32
          CASE_NUMERIC (UINT64, 0);
      
#else   /*  */
#pragma message ("Check if cast from uint64 to double is supported with msvc 6.0")
#endif
          CASE_NUMERIC (INT64, 0);

#undef CASE_NUMERIC

    case G_TYPE_STRING:
      if (strstr(param->name,"ocation")||strstr(param->name,"ilename")||strstr(param->name,"uri")){
	gtk_widget_show(pview->file);
	gtk_widget_show(pview->filetext);	
      }
      else gtk_widget_show (pview->entry);
      break;
    default:
      if (G_IS_PARAM_SPEC_ENUM (param)) {
        GtkWidget *menu, *w;
        gint i;
        gchar *str;
        GEnumClass *class = G_ENUM_CLASS (g_type_class_ref (param->value_type));

        menu = gtk_menu_new ();

        if (pview->enum_values)
          g_free (pview->enum_values);
        pview->enum_values = g_new0 (gint, class->n_values);

        for (i = 0; i < class->n_values; i++) {
          GEnumValue *value = &class->values[i];

          pview->enum_values[i] = value->value;
          str = g_strdup_printf ("%s (%d)", value->value_nick, value->value);
          w = gtk_menu_item_new_with_label (str);
          g_free (str);
          gtk_widget_show (w);
          gtk_menu_append (GTK_MENU (menu), w);
        }

        gtk_option_menu_set_menu (GTK_OPTION_MENU (pview->optionmenu), menu);

        gtk_widget_show (pview->optionmenu);
      } else if (!strcmp("GstCaps",g_type_name (pview->param->value_type))){
	gtk_widget_show (pview->entry);
      } else if (!strcmp("GstPad",g_type_name (pview->param->value_type))){
	//guess which of the Pads are Request pads, or take all
	//
        GtkWidget *menu, *w;
        gint i=0;//iterator
        gchar *str;
        //GEnumClass *class = G_ENUM_CLASS (g_type_class_ref (param->value_type));

        menu = gtk_menu_new ();

        if (pview->enum_pointer)
          g_free (pview->enum_pointer);

        pview->enum_pointer = g_new0 (gpointer, 1+element->numpads);//save gpointers inside the elements
	GstIterator* padit=gst_element_iterate_pads (element);
	gboolean done=FALSE;
	gpointer item;
	while (!done) {
	  switch (gst_iterator_next (padit, (gpointer*)&item)) {
	  case GST_ITERATOR_OK:
		pview->enum_pointer[i++]=item;
		str = g_strdup (gst_pad_get_name((GstPad*)item));
		//g_print("Adding pad pointer %p, Name %s\n",item,str);
                w = gtk_menu_item_new_with_label (str);
                g_free (str);
                gtk_widget_show (w);
		gtk_menu_append (GTK_MENU (menu), w);
		gst_object_unref (item);
		break;
	  case GST_ITERATOR_RESYNC:
		gst_iterator_free (padit);
		padit=gst_element_iterate_pads (element);
		i=0;
		break;
	  case GST_ITERATOR_ERROR:
		//g_print("GstIterator Returning Unexpected Error");
		done = TRUE;
		break;
	  case GST_ITERATOR_DONE:
		//g_print("GstIterator Done");
		done = TRUE;
		break;
	  default:
		//g_print("GstIterator Returning Unexpected Error"); 
		done = TRUE;
		break;
	  }
	}
	gst_iterator_free (padit);
        w = gtk_menu_item_new_with_label ("NULL");
        gtk_widget_show (w);
        gtk_menu_append (GTK_MENU (menu), w);

        gtk_option_menu_set_menu (GTK_OPTION_MENU (pview->optionmenu), menu);
        gtk_widget_show (pview->optionmenu);	
      } else {
        g_warning ("param_changed for type %s not yet implemented",
            g_type_name (param->value_type));
      }
  }

  unblock_signals (pview);

  gst_element_ui_prop_view_update (pview);
}

static void
pview_value_changed (GstElementUIPropView * pview)
{
  GParamSpec *param = pview->param;

  GST_DEBUG ("value changed for pview %s, setting element property",
      param->name);

  switch (param->value_type) {
    case G_TYPE_BOOLEAN:
      g_object_set (G_OBJECT (pview->element), param->name, pview->on_pending,
          NULL);
      break;
    case G_TYPE_STRING:
      if (strstr(param->name,"ocation")||strstr(param->name,"ilename")||strstr(param->name,"uri") ){
        g_object_set (G_OBJECT (pview->element), param->name,
            gtk_entry_get_text (GTK_ENTRY (pview->filetext)), NULL);
	}
      else{
        g_object_set (G_OBJECT (pview->element), param->name,
            gtk_entry_get_text (GTK_ENTRY (pview->entry)), NULL);
	}
      break;

#define CASE_NUMERIC(type_lower, type_upper)						\
    case G_TYPE_##type_upper:								\
        g_object_set (G_OBJECT (pview->element), param->name, 					\
                      (type_lower) GTK_ADJUSTMENT (pview->adjustment)->value, NULL);	\
        break

      CASE_NUMERIC (gfloat, FLOAT);
      CASE_NUMERIC (gdouble, DOUBLE);
      CASE_NUMERIC (gint, INT);
      CASE_NUMERIC (guint, UINT);
      CASE_NUMERIC (glong, LONG);
      CASE_NUMERIC (gulong, ULONG);
      CASE_NUMERIC (gint64, INT64);
      CASE_NUMERIC (guint64, UINT64);

#undef CASE_NUMERIC

    default:
      if (G_IS_PARAM_SPEC_ENUM (param)) {
        g_object_set (G_OBJECT (pview->element), param->name,
            pview->
            enum_values[gtk_option_menu_get_history (GTK_OPTION_MENU (pview->
                        optionmenu))], NULL);
	//GstCaps
      } else if (!strcmp("GstCaps",g_type_name (pview->param->value_type))){
	GstCaps *temp=gst_caps_from_string (gtk_entry_get_text (GTK_ENTRY (pview->entry)));
	if (temp) g_object_set (G_OBJECT (pview->element), param->name,temp, NULL);
      } else if (!strcmp("GstPad",g_type_name (pview->param->value_type))){
	GstPad *temp=(GstPad*)pview->enum_pointer[gtk_option_menu_get_history (GTK_OPTION_MENU (pview->
                        optionmenu))];
	g_object_set (G_OBJECT (pview->element), param->name,temp, NULL);
	gst_element_ui_prop_view_update (pview);
      } else {
        g_warning ("value_changed for type %s not yet implemented",
            g_type_name (param->value_type));
      }
  }

//    gst_element_ui_prop_view_update (pview);
}

static void
on_location_hit (GtkWidget * widget, GstElementUIPropView * pview)
{

  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new ("Choose File",
				      NULL,
				      GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);
  if (strstr(pview->param->name,"uri")) gtk_file_chooser_select_filename(dialog,gtk_entry_get_text(pview->filetext)+7); 
  else gtk_file_chooser_select_filename(dialog,gtk_entry_get_text(pview->filetext));
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    char *filename;

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    char* filenamefull=g_malloc(strlen(filename)+10);
    if (strstr(pview->param->name,"uri")) sprintf(filenamefull,"file://%s",filename);
    else sprintf(filenamefull,"%s",filename);
    g_object_set (G_OBJECT (pview->element), pview->param->name,
          filenamefull, NULL);
    gtk_entry_set_text(pview->filetext,filenamefull); 
    g_free (filenamefull);
    g_free (filename);
  }
  gtk_widget_destroy (dialog);

}




static void
on_adjustment_value_changed (GtkAdjustment * adjustment,
    GstElementUIPropView * pview)
{
  GST_DEBUG ("on_adjustment_value_changed: %f", adjustment->value);
  pview_value_changed (pview);
}

static void
on_toggle_button_toggled (GtkToggleButton * button,
    GstElementUIPropView * pview)
{
  pview->on_pending = !pview->on_pending;
  pview_value_changed (pview);
}

static void
on_entry_activate (GtkEntry * entry, GstElementUIPropView * pview)
{
  pview_value_changed (pview);
}

static void
on_optionmenu_changed (GtkOptionMenu * option, GstElementUIPropView * pview)
{
  pview_value_changed (pview);
}

static void
block_signals (GstElementUIPropView * pview)
{
  g_signal_handlers_block_by_func ((GObject *) pview->adjustment,
      G_CALLBACK (on_adjustment_value_changed), pview);
  g_signal_handlers_block_by_func ((GObject *) pview->toggle_on,
      G_CALLBACK (on_toggle_button_toggled), pview);
  g_signal_handlers_block_by_func ((GObject *) pview->toggle_off,
      G_CALLBACK (on_toggle_button_toggled), pview);
  g_signal_handlers_block_by_func ((GObject *) pview->toggle_off,
      G_CALLBACK (on_entry_activate), pview);
  g_signal_handlers_block_by_func ((GObject *) pview->optionmenu,
      G_CALLBACK (on_optionmenu_changed), pview);
}

static void
unblock_signals (GstElementUIPropView * pview)
{
  g_signal_handlers_unblock_by_func ((GObject *) pview->optionmenu,
      G_CALLBACK (on_optionmenu_changed), pview);
  g_signal_handlers_unblock_by_func ((GObject *) pview->toggle_off,
      G_CALLBACK (on_toggle_button_toggled), pview);
  g_signal_handlers_unblock_by_func ((GObject *) pview->toggle_on,
      G_CALLBACK (on_toggle_button_toggled), pview);
  g_signal_handlers_unblock_by_func ((GObject *) pview->adjustment,
      G_CALLBACK (on_adjustment_value_changed), pview);
  g_signal_handlers_unblock_by_func ((GObject *) pview->adjustment,
      G_CALLBACK (on_entry_activate), pview);
}

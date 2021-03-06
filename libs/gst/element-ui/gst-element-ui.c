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
#include "gst-element-ui.h"

static void gst_element_ui_class_init (GstElementUIClass * klass);
static void gst_element_ui_init (GstElementUI * ui);
static void gst_element_ui_on_element_notify (GObject * object,
    GParamSpec * pspec, GstElementUI * ui);
static void gst_element_ui_on_element_dispose (GstElementUI * ui,
    GstElement * destroyed_element);
static void gst_element_ui_dispose (GObject * object);

static void gst_element_ui_reset (GstElementUI * ui);

static void gst_element_ui_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_element_ui_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void on_combobox_changed (GtkComboBox * combobox, gpointer user_data);

static gboolean offset_hack (GstElementUI * ui);

static gpointer parent_class = NULL;

enum
{
  PROP_0,
  PROP_ELEMENT,
  PROP_VIEW_MODE,
  PROP_SHOW_READONLY,
  PROP_SHOW_WRITEONLY,
  PROP_EXCLUDE_STRING
};

#ifdef _MSC_VER
//static void debug(text, ...) {}
# ifdef _DEBUG
#  define debug g_message
# else
#  define debug
# endif
#endif

/* #define _DEBUG */
#ifdef G_HAVE_ISO_VARARGS

#ifdef _DEBUG
#define debug(...) g_message(__VA_ARGS__)
#else
#define debug(...)
#endif

#elif defined(G_HAVE_GNUC_VARARGS)

#ifdef _DEBUG
#define debug(text, args...) g_message(text, ##args)
#else
#define debug(text, args...)
#endif

#endif

#define VIEW_MODE (view_mode_get_type())
static GType
view_mode_get_type (void)
{
  static GType data_type = 0;
  static GEnumValue data[] = {
    {GST_ELEMENT_UI_VIEW_MODE_COMPACT, "1",
        "Compact view mode (One property at a time)"},
    {GST_ELEMENT_UI_VIEW_MODE_FULL, "2", "Extended view mode (All properties)"},
    {0, NULL, NULL},
  };

  if (!data_type) {
    data_type = g_enum_register_static ("GstElementUIViewMode", data);
  }
  return data_type;
}

GType
gst_element_ui_get_type (void)
{
  static GType element_ui_type = 0;

  if (!element_ui_type) {
    static const GTypeInfo element_ui_info = {
      sizeof (GstElementUIClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_element_ui_class_init,
      NULL,
      NULL,
      sizeof (GstElementUI),
      0,
      (GInstanceInitFunc) gst_element_ui_init,
    };
    element_ui_type = g_type_register_static (
        GTK_TYPE_GRID, "GstElementUI", &element_ui_info, 0);
  }
  return element_ui_type;
}

static void
gst_element_ui_class_init (GstElementUIClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /* 'changed' is for when the structure of the elementui's gtkadjustment
     changes, as in setting a new upper bound. 'value_changed' is for when the
     adjustment is changed by the user. we connect to 'notify' signal on the
     element objects themselves to update elements of the gui when the element
     itself changes. got it? */

  object_class->dispose = gst_element_ui_dispose;

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = gst_element_ui_set_property;
  object_class->get_property = gst_element_ui_get_property;

  g_object_class_install_property (object_class, PROP_ELEMENT,
      g_param_spec_object ("element", "Element", "Element",
          gst_element_get_type (), G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_VIEW_MODE,
      g_param_spec_enum ("view-mode", "View mode", "View mode",
          view_mode_get_type (), GST_ELEMENT_UI_VIEW_MODE_COMPACT,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SHOW_READONLY,
      g_param_spec_boolean ("show-readonly", "Show readonly?",
          "Show readonly properties?", TRUE,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SHOW_WRITEONLY,
      g_param_spec_boolean ("show-writeonly", "Show writeonly",
          "Show writeonly properties?", FALSE,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_EXCLUDE_STRING,
      g_param_spec_string ("exclude-string", "Exclude string",
          "Exclude properties that are substrings of this string", NULL,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
gst_element_ui_init (GstElementUI * ui)
{
  gtk_grid_set_row_spacing (GTK_GRID (ui), 2);
  gtk_grid_set_column_spacing (GTK_GRID (ui), 2);

  ui->name = gtk_label_new ("No element selected");
  g_object_ref (G_OBJECT (ui->name));
  gtk_widget_show (ui->name);
  gtk_widget_set_hexpand (ui->name, TRUE);
  gtk_label_set_justify (GTK_LABEL (ui->name), GTK_JUSTIFY_RIGHT);
  gtk_widget_set_size_request (ui->name, 150, -1);
  gtk_misc_set_padding (GTK_MISC (ui->name), 2, 0);

  ui->combobox = gtk_combo_box_text_new ();
  g_object_ref (G_OBJECT (ui->combobox));
  gtk_widget_set_hexpand (ui->combobox, TRUE);

  g_signal_connect (ui->combobox, "changed",
      G_CALLBACK (on_combobox_changed), ui);

  gst_element_ui_reset (ui);
}

GstElementUI *
gst_element_ui_new (GstElement * element)
{
  GstElementUI *ui =
      GST_ELEMENT_UI (g_object_new (GST_TYPE_ELEMENT_UI, "element", element,
          NULL));

  return ui;
}

static void
gst_element_ui_reset (GstElementUI * ui)
{
  GList *children = gtk_container_get_children (GTK_CONTAINER (ui));
  GList *cur;

  for (cur = children; cur; cur = g_list_next (cur))
    gtk_container_remove (GTK_CONTAINER (ui),
        GTK_WIDGET (cur->data));

  g_list_free (children);

  /*
   * We keep a reference on the GtkLabel and GtkComboBoxText,
   * so they have not been destroyed.
   */
  gtk_grid_attach (GTK_GRID (ui), ui->name, 0, 0, 2, 1);
  gtk_grid_attach (GTK_GRID (ui), ui->combobox, 0, 1, 2, 1);
}

static void
gst_element_ui_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstElementUI *ui;
  guint i = 0, j = 0, nprops;
  GParamSpec **params;
  GstElementFactory *factory;
  gchar *str;

  ui = GST_ELEMENT_UI (object);

  switch (prop_id) {
    case PROP_ELEMENT:
      debug ("element set");
      if (ui->element) {
        debug ("doing cleanup");
        g_signal_handlers_disconnect_by_func (ui->element,
            gst_element_ui_on_element_notify, ui);
        if (ui->nprops) {
          for (i = 0; i < ui->nprops; i++) {
            gtk_widget_destroy (ui->plabels[i]);
            gtk_widget_destroy (GTK_WIDGET (ui->pviews[i]));
          }
          g_free (ui->pviews);
          ui->pviews = NULL;
          g_free (ui->plabels);
          ui->plabels = NULL;
          ui->nprops = 0;
        } else {
          gtk_widget_destroy (ui->message);
          ui->message = NULL;
        }
      }
      debug ("building new ui");

      ui->element = (GstElement *) g_value_get_object (value);

      if (!ui->element) {
        gtk_label_set_text (GTK_LABEL (ui->name), "No element selected");
        return;
      }
#define SHOWABLE_PARAMETER(p)                                                   \
	(((p->flags & G_PARAM_READABLE && p->flags & G_PARAM_WRITABLE)          \
          || (p->flags & G_PARAM_READABLE && ui->show_readonly)                 \
          || (p->flags & G_PARAM_WRITABLE && ui->show_writeonly))               \
         && !(ui->exclude_string && strstr (ui->exclude_string, p->name)))

      params =
          g_object_class_list_properties (G_OBJECT_GET_CLASS (ui->element),
          &nprops);
      for (i = 0; i < nprops; i++)
        if (!SHOWABLE_PARAMETER (params[i]))
          j++;

      ui->nprops = nprops - j;

      if (ui->nprops) {
        GtkTreeModel *combobox_store;

        ui->params = g_new (GParamSpec *, ui->nprops);

        j = 0;
        for (i = 0; i < nprops; i++)
          if (SHOWABLE_PARAMETER (params[i]))
            ui->params[j++] = params[i];

        // clears the combo box
        combobox_store =
            gtk_combo_box_get_model (GTK_COMBO_BOX (ui->combobox));
        if (combobox_store)
          gtk_list_store_clear (GTK_LIST_STORE (combobox_store));

        for (j = 0; j < ui->nprops; j++)
          gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (ui->combobox),
              ui->params[j]->name);
        ui->selected = 0;

        if (ui->view_mode == GST_ELEMENT_UI_VIEW_MODE_COMPACT)
          gtk_widget_show (ui->combobox);

        gst_element_ui_reset (ui);

        ui->pviews = g_new0 (GstElementUIPropView *, ui->nprops);
        ui->plabels = g_new0 (GtkWidget *, ui->nprops);
        for (i = 0; i < ui->nprops; i++) {
          ui->pviews[i] = g_object_new (gst_element_ui_prop_view_get_type (),
              "element", ui->element, "param", ui->params[i], NULL);
          gtk_grid_attach (GTK_GRID (ui), GTK_WIDGET (ui->pviews[i]),
              1, i + 2, 1, 1);
          gtk_widget_set_hexpand (GTK_WIDGET (ui->pviews[i]), TRUE);

          str = g_strconcat (ui->params[i]->name, ":", NULL);
          ui->plabels[i] = gtk_label_new (str);
          g_free (str);
          gtk_grid_attach (GTK_GRID (ui), ui->plabels[i],
              0, i + 2, 1, 1);
        }

        if (g_object_class_find_property (G_OBJECT_GET_CLASS (ui->element),
                "filesize")
            && g_object_class_find_property (G_OBJECT_GET_CLASS (ui->element),
                "offset"))
          offset_hack (ui);

        g_signal_connect (G_OBJECT (ui->element), "notify",
            G_CALLBACK (gst_element_ui_on_element_notify), ui);
        gst_element_ui_prop_view_update (ui->pviews[ui->selected]);
      } else {
        gst_element_ui_reset (ui);
        ui->message = gtk_label_new ("This element has no properties.");
        gtk_widget_show (ui->message);
        gtk_grid_attach (GTK_GRID (ui), ui->message,
            0, 2, 2, 1);
      }

      factory = gst_element_get_factory (ui->element);
      str = g_strconcat (
          gst_element_factory_get_metadata (factory, GST_ELEMENT_METADATA_LONGNAME),
          ": ",
          gst_object_get_name (GST_OBJECT (ui->element)), NULL);
      gtk_label_set_text (GTK_LABEL (ui->name), str);
      g_free (str);

      g_object_weak_ref (G_OBJECT (ui->element),
          (GWeakNotify) gst_element_ui_on_element_dispose, ui);
      debug ("done setting element");

      break;
    case PROP_VIEW_MODE:
      ui->view_mode = g_value_get_enum (value);
      break;
    case PROP_SHOW_READONLY:
      /* fixme: must be set before element is set */
      ui->show_readonly = g_value_get_boolean (value);
      break;
    case PROP_SHOW_WRITEONLY:
      /* fixme: must be set before element is set */
      ui->show_writeonly = g_value_get_boolean (value);
      break;
    case PROP_EXCLUDE_STRING:
      /* fixme: must be set before element is set */
      if (ui->exclude_string)
        g_free (ui->exclude_string);

      ui->exclude_string = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }

  if (ui->view_mode == GST_ELEMENT_UI_VIEW_MODE_COMPACT) {
    for (i = 0; i < ui->nprops; i++) {
      gtk_widget_hide (ui->plabels[i]);
      gtk_widget_hide (GTK_WIDGET (ui->pviews[i]));
    }
    gtk_widget_show (GTK_WIDGET (ui->pviews[ui->selected]));
    gtk_widget_show (ui->combobox);
  } else {
    for (i = 0; i < ui->nprops; i++) {
      gtk_widget_show (ui->plabels[i]);
      gtk_widget_show (GTK_WIDGET (ui->pviews[i]));
    }
    gtk_widget_hide (ui->combobox);
  }
}

static void
gst_element_ui_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstElementUI *ui;

  ui = GST_ELEMENT_UI (object);

  switch (prop_id) {
    case PROP_ELEMENT:
      g_value_set_object (value, G_OBJECT (ui->element));
      break;
    case PROP_VIEW_MODE:
      g_value_set_enum (value, ui->view_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_element_ui_on_element_dispose (GstElementUI * ui,
    GstElement * destroyed_element)
{
  g_object_set (ui, "element", NULL, NULL);
}

static void
gst_element_ui_dispose (GObject * object)
{
  GstElementUI *ui = GST_ELEMENT_UI (object);
  gint i;

  if (ui->element) {
    for (i = 0; i < ui->nprops; i++)
      gtk_widget_destroy (GTK_WIDGET (ui->pviews[i]));
    g_free (ui->pviews);
    ui->pviews = NULL;
    ui->nprops = 0;
    ui->element = NULL;
  }

  if (G_OBJECT_CLASS (parent_class)->dispose)
    (*G_OBJECT_CLASS (parent_class)->dispose) (object);
}

static void
gst_element_ui_on_element_notify (GObject * object, GParamSpec * param,
    GstElementUI * ui)
{
  gint i = 0;
  gboolean handled = FALSE;

  debug ("notify from gst object %s, param %s",
      gst_object_get_name (GST_OBJECT (object)), param->name);

  for (i = 0; i < ui->nprops; i++) {
    if (strcmp (param->name, ui->params[i]->name) == 0) {
      gst_element_ui_prop_view_update_async (ui->pviews[i]);
      handled = TRUE;
    }
  }

  /* hackeroo */
  if (strcmp (param->name, "filesize") == 0)
    g_timeout_add (0, (GSourceFunc) offset_hack, ui);

  if (handled)
    return;

  debug ("ignoring property %s", param->name);
  /* it's possible that some props we excluded can show up here, just let them
     slide */
}

/* set the upper bound of an offset prop-view to ::filesize */
static gboolean
offset_hack (GstElementUI * ui)
{
  gint i;
  gint64 my_int64;
  GstElementUIPropView *pview = NULL;
  GtkAdjustment *adj;
  gchar *lower, *upper;

  for (i = 0; i < ui->nprops; i++) {
    if (strcmp ("offset", ui->params[i]->name) == 0) {
      pview = GST_ELEMENT_UI_PROP_VIEW (ui->pviews[i]);
    }
  }

  g_return_val_if_fail (pview != NULL, FALSE);

  adj = GTK_ADJUSTMENT (pview->adjustment);
  gtk_adjustment_set_lower (adj, G_PARAM_SPEC_INT64 (pview->param)->minimum);
  g_object_set (G_OBJECT (ui->element), "filesize", &my_int64, NULL);
  gtk_adjustment_set_upper (adj, gtk_adjustment_get_lower (adj) + my_int64);
  debug ("setting upper = %lld = %f", my_int64, gtk_adjustment_get_upper (adj));
  gtk_adjustment_set_step_increment (adj,
      (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_lower (adj)) / 20);
  gtk_adjustment_set_page_increment (adj,
      (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_lower (adj)) / 4);
  gtk_adjustment_changed (adj);

  lower = g_strdup_printf ("%.4g", gtk_adjustment_get_lower (adj));
  upper = g_strdup_printf ("%.4g", gtk_adjustment_get_upper (adj));
  gtk_label_set_text (GTK_LABEL (pview->label_lower), lower);
  gtk_label_set_text (GTK_LABEL (pview->label_upper), upper);
  g_free (lower);
  g_free (upper);

  return FALSE;
}

static void
on_combobox_changed (GtkComboBox * combobox, gpointer user_data)
{
  GstElementUI *element_ui = GST_ELEMENT_UI (user_data);

  if (element_ui->view_mode == GST_ELEMENT_UI_VIEW_MODE_COMPACT)
    gtk_widget_hide (GTK_WIDGET (element_ui->pviews[element_ui->selected]));

  element_ui->selected =
      gtk_combo_box_get_active (combobox);
  debug ("selected: %d", element_ui->selected);

  if (element_ui->selected < 0)
    return;

  if (element_ui->view_mode == GST_ELEMENT_UI_VIEW_MODE_COMPACT)
    gtk_widget_show (GTK_WIDGET (element_ui->pviews[element_ui->selected]));
  gst_element_ui_prop_view_update (element_ui->pviews[element_ui->selected]);
}

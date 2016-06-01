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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif  /*  */

#include <glib/gi18n-lib.h>
#include "browser.h"
#include "element-tree.h"
static void gst_element_browser_init (GstElementBrowser * browser);
static void gst_element_browser_class_init (GstElementBrowserClass * browser);

static void on_tree_selection_changed (GObject * object,
    GstElementBrowser * browser);
static void on_tree_activated (GObject * object, GstElementBrowser * browser);

static GtkDialogClass *parent_class = NULL;


GType
gst_element_browser_get_type (void)
{
  static GType element_browser_type = 0;

  if (!element_browser_type) {
    static const GTypeInfo element_browser_info = {
      sizeof (GstElementBrowserClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_element_browser_class_init,
      NULL,
      NULL,
      sizeof (GstElementBrowser),
      0,
      (GInstanceInitFunc) gst_element_browser_init,
    };
    element_browser_type =
        g_type_register_static (gtk_dialog_get_type (), "GstElementBrowser",
        &element_browser_info, 0);
  }
  return element_browser_type;
}

static void
gst_element_browser_class_init (GstElementBrowserClass * klass)
{
  parent_class = (GtkDialogClass *) g_type_class_ref (gtk_dialog_get_type ());
}

static void
gst_element_browser_init (GstElementBrowser * browser)
{
  GtkDialog *dialog;
  GtkWidget *tree;
  GtkTable *table;
  GtkWidget *vbox, *hpaned;
  GtkWidget *frame;

  GtkWidget *longname, *author, *description;

  dialog = GTK_DIALOG (browser);

  gtk_dialog_add_buttons (dialog,
      GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
  gtk_dialog_set_default_response (dialog, GTK_RESPONSE_CANCEL);

  gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 550, 400);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Select element..."));

  hpaned = gtk_hpaned_new ();
  gtk_box_pack_start (GTK_BOX (dialog->vbox), hpaned, TRUE, TRUE, 0);

  tree = g_object_new (gst_element_browser_element_tree_get_type (), NULL);
  gtk_widget_set_size_request (GTK_WIDGET (tree), 200, -1);
  gtk_paned_pack1 (GTK_PANED (hpaned), tree, FALSE, TRUE);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_size_request (vbox, 100, -1);
  gtk_paned_pack2 (GTK_PANED (hpaned), vbox, TRUE, TRUE);

  frame = gtk_frame_new ("Element Details");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  /* create the details table and put a title on it */
  table = GTK_TABLE (gtk_table_new (2, 3, FALSE));

  /* the long name of the element */
  longname = gtk_label_new ("Name:");
  gtk_misc_set_alignment (GTK_MISC (longname), 1.0, 0.0);
  gtk_table_attach (table, longname, 0, 1, 0, 1, GTK_FILL, 0, 5, 0);
  browser->longname = GTK_WIDGET (g_object_new (GTK_TYPE_LABEL,
          "selectable", TRUE,
          "wrap", TRUE,
          "justify", GTK_JUSTIFY_LEFT, "xalign", 0.0, "yalign", 0.0, NULL));
  gtk_table_attach (table, browser->longname, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND,
      0, 0, 0);

  /* the description */
  description = gtk_label_new ("Description:");
  gtk_misc_set_alignment (GTK_MISC (description), 1.0, 0.0);
  gtk_table_attach (table, description, 0, 1, 1, 2, GTK_FILL, 0, 5, 0);
  browser->description = GTK_WIDGET (g_object_new (GTK_TYPE_LABEL,
          "selectable", TRUE,
          "wrap", TRUE,
          "justify", GTK_JUSTIFY_LEFT, "xalign", 0.0, "yalign", 0.0, NULL));
  gtk_table_attach (table, browser->description, 1, 2, 1, 2,
      GTK_FILL | GTK_EXPAND, 0, 0, 0);

  /* the author */
  author = gtk_label_new ("Author:");
  gtk_misc_set_alignment (GTK_MISC (author), 1.0, 0.0);
  gtk_table_attach (table, author, 0, 1, 3, 4, GTK_FILL, 0, 5, 0);
  browser->author = GTK_WIDGET (g_object_new (GTK_TYPE_LABEL,
          "selectable", TRUE,
          "wrap", TRUE,
          "justify", GTK_JUSTIFY_LEFT, "xalign", 0.0, "yalign", 0.0, NULL));
  gtk_table_attach (table, browser->author, 1, 2, 2, 3, GTK_FILL | GTK_EXPAND,
      0, 0, 0);

  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (table));

  frame = gtk_frame_new ("Pads");
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

  browser->pads = gst_element_browser_caps_tree_new ();
  gtk_container_add (GTK_CONTAINER (frame), browser->pads);

  frame = gtk_frame_new ("Pad templates");
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

  browser->padtemplates = gst_element_browser_caps_tree_new ();
  gtk_container_add (GTK_CONTAINER (frame), browser->padtemplates);

  g_signal_connect (G_OBJECT (tree), "element-selected",
      G_CALLBACK (on_tree_selection_changed), browser);
  g_signal_connect (G_OBJECT (tree), "element-activated",
      G_CALLBACK (on_tree_activated), browser);

  gtk_widget_show_all (GTK_WIDGET (browser));
  gtk_widget_hide (GTK_WIDGET (browser));
}

GtkWidget *
gst_element_browser_new ()
{
  return GTK_WIDGET (g_object_new (gst_element_browser_get_type (), NULL));
}

GstElementFactory *
gst_element_browser_pick_modal ()
{
  static GstElementBrowser *browser = NULL;
  gint response;

  if (!browser)
    browser = GST_ELEMENT_BROWSER (gst_element_browser_new ());

  response = gtk_dialog_run (GTK_DIALOG (browser));

  gtk_widget_hide (GTK_WIDGET (browser));

  if (response != GTK_RESPONSE_ACCEPT)
    return NULL;
  else
    return browser->selected;
}

static void
on_tree_activated (GObject * object, GstElementBrowser * browser)
{
  GstElementFactory *factory;
  GstElementBrowserElementTree *element_tree =
      GST_ELEMENT_BROWSER_ELEMENT_TREE (object);

  g_object_get (element_tree, "selected", &factory, NULL);
  browser->selected = factory;

  gtk_dialog_response (GTK_DIALOG (browser), GTK_RESPONSE_ACCEPT);
}

static void
on_tree_selection_changed (GObject * object, GstElementBrowser * browser)
{
  GstElementFactory *factory;
  GstElementBrowserElementTree *element_tree =
      GST_ELEMENT_BROWSER_ELEMENT_TREE (object);

  g_object_get (element_tree, "selected", &factory, NULL);
  browser->selected = factory;

  g_return_if_fail (factory != NULL);

  gtk_label_set_text (GTK_LABEL (browser->longname), factory->details.longname);
  gtk_label_set_text (GTK_LABEL (browser->description),
      factory->details.description);
  gtk_label_set_text (GTK_LABEL (browser->author), factory->details.author);

  g_object_set (G_OBJECT (browser->padtemplates), "element-factory",
      browser->selected, NULL);

  if (browser->element)
    gst_object_unref (GST_OBJECT (browser->element));

  browser->element = gst_element_factory_create (browser->selected, NULL);
  g_object_set (G_OBJECT (browser->pads), "element", browser->element, NULL);
}

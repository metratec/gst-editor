#include "stockicons.h"
#include <pixmaps/pixmaps.h>

/* add Icons with given size */
static void
add_sized (GtkIconFactory * factory, const guchar * inline_data,
    GtkIconSize size, const gchar * stock_id)
{
  GtkIconSet *set;
  GtkIconSource *source;
  GdkPixbuf *pixbuf;

  pixbuf = gdk_pixbuf_new_from_inline (-1, inline_data, FALSE, NULL);
  source = gtk_icon_source_new ();
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, size);

  set = gtk_icon_set_new ();
  gtk_icon_set_add_source (set, source);

  gtk_icon_factory_add (factory, stock_id, set);

  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_source_free (source);
  gtk_icon_set_unref (set);
}

#define ADD_SIZED(lower_case, upper_case, size) \
  add_sized (factory, gst_editor_stock_##lower_case, size, GST_EDITOR_STOCK_##upper_case)

/* add our default icons */
static void
add_default_icons (GtkIconFactory * factory)
{
  add_sized (factory, gst_editor_stock_image, GTK_ICON_SIZE_DIALOG,
      GST_EDITOR_STOCK_LOGO);
}

/* init our stock items */
void
_gst_editor_stock_icons_init (void)
{
  static gboolean initialized = FALSE;
  GtkIconFactory *factory;

  if (initialized)
    return;
  else
    initialized = TRUE;

  factory = gtk_icon_factory_new ();
  add_default_icons (factory);
  gtk_icon_factory_add_default (factory);
}

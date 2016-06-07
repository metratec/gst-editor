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
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gst/gst.h>

#include <sys/stat.h>
#include <unistd.h>

#include "gsteditorimage.h"

GHashTable *_gst_editor_images;

static char *_gst_editor_image_name[] = {
  "pixmaps/bin.xpm",
  "pixmaps/pipeline.xpm",
  "pixmaps/thread.xpm",
  "pixmaps/element.xpm",
  "pixmaps/sink.xpm",
  "pixmaps/src.xpm",
  "pixmaps/connection.xpm",
  "pixmaps/filter.xpm",
  "pixmaps/tee.xpm",
};

struct _image_entry
{
  GstEditorImageType type;
  GType (*gtype) (void);
};

static const struct _image_entry _image_types[] = {
  {GST_EDITOR_IMAGE_BIN, gst_bin_get_type},
//  {GST_EDITOR_IMAGE_THREAD, gst_thread_get_type},
  {GST_EDITOR_IMAGE_PIPELINE, gst_pipeline_get_type}
};

GstEditorImage *
gst_editor_image_get (GstEditorImageType type)
{
  struct stat statbuf;
  GstEditorImage *new = g_new0 (GstEditorImage, 1);

  if (stat (_gst_editor_image_name[type], &statbuf) == 0) {
    new->pixmap =
        gdk_pixmap_colormap_create_from_xpm (NULL, gdk_colormap_get_system (),
        &new->bitmap, NULL, _gst_editor_image_name[type]);
  } else {
    new->pixmap =
        gdk_pixmap_colormap_create_from_xpm (NULL, gdk_colormap_get_system (),
        &new->bitmap, NULL, g_strconcat (DATADIR, _gst_editor_image_name[type],
            NULL));
  }

  return new;
}

GstEditorImage *
gst_editor_image_get_for_type (GType type)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (_image_types); i++) {
    if (_image_types[i].gtype () == type) {
      return gst_editor_image_get (_image_types[i].type);
    }
  }
  return gst_editor_image_get (GST_EDITOR_IMAGE_ELEMENT);
}

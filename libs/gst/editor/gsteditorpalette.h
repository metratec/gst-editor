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


#ifndef __GST_EDITOR_PALETTE_H__
#define __GST_EDITOR_PALETTE_H__

#include <gst/gst.h>
#include <glade/glade.h>
#include <gst/editor/editor.h>

#define GST_TYPE_EDITOR_PALETTE (gst_editor_palette_get_type())
#define GST_EDITOR_PALETTE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_EDITOR_PALETTE, GstEditorPalette))
#define GST_EDITOR_PALETTE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_EDITOR_PALETTE, GstEditorPaletteClass))
#define GST_IS_EDITOR_PALETTE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_EDITOR_PALETTE))
#define GST_IS_EDITOR_PALETTE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_EDITOR_PALETTE))
#define GST_EDITOR_PALETTE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_EDITOR_PALETTE, GstEditorPaletteClass))

typedef struct _GstEditorPalette GstEditorPalette;
typedef struct _GstEditorPaletteClass GstEditorPaletteClass;

struct _GstEditorPalette
{
  GObject object;

  GstEditorCanvas *canvas;

  GladeXML *xml;
  GtkWidget *window;
  GtkWidget *element_tree;
  GtkWidget *debug_ui;
};

struct _GstEditorPaletteClass
{
  GObjectClass parent_class;
};

GType gst_editor_palette_get_type (void);


#endif /* __GST_EDITOR_PALETTE_H__ */

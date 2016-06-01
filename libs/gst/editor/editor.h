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


#ifndef __GST_EDITOR_MAIN_H__
#define __GST_EDITOR_MAIN_H__


#include <gst/gst.h>

/* FIXME: how does gtk+ manage to do this? */

typedef struct _GstEditor GstEditor;
typedef struct _GstEditorClass GstEditorClass;
typedef struct _GstEditorBin GstEditorBin;
typedef struct _GstEditorBinClass GstEditorBinClass;
typedef struct _GstEditorCanvas GstEditorCanvas;
typedef struct _GstEditorCanvasClass GstEditorCanvasClass;
typedef struct _GstEditorLink GstEditorLink;
typedef struct _GstEditorLinkClass GstEditorLinkClass;
typedef struct _GstEditorItem GstEditorItem;
typedef struct _GstEditorItemClass GstEditorItemClass;
typedef struct _GstEditorElement GstEditorElement;
typedef struct _GstEditorElementClass GstEditorElementClass;
typedef struct _GstEditorPad GstEditorPad;
typedef struct _GstEditorPadClass GstEditorPadClass;
typedef GstEditorPad GstEditorPadAlways;
typedef GstEditorPadClass GstEditorPadAlwaysClass;
typedef GstEditorPad GstEditorPadSometimes;
typedef GstEditorPadClass GstEditorPadSometimesClass;
typedef GstEditorPad GstEditorPadRequest;
typedef GstEditorPadClass GstEditorPadRequestClass;
typedef GstEditorPad GstEditorPadRequested;
typedef GstEditorPadClass GstEditorPadRequestedClass;
typedef GstEditorPad GstEditorPadGhost;
typedef GstEditorPadClass GstEditorPadGhostClass;

#include <gst/editor/gsteditor.h>
#include <gst/editor/gsteditorbin.h>
#include <gst/editor/gsteditorcanvas.h>
#include <gst/editor/gsteditorlink.h>
#include <gst/editor/gsteditoritem.h>
#include <gst/editor/gsteditorelement.h>
#include <gst/editor/gsteditorpad.h>

void gste_init (void);

#endif /* __GST_EDITOR_H__ */

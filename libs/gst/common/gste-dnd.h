/* gst-editor
 * Copyright (C) <2001> Steve Baker <stevebaker_org@yahoo.co.uk>
 * Copyright (C) <2002, 2003> Andy Wingo <wingo at pobox dot com>
 * Copyright (C) <2004> Jan Schmidt <thaytan@mad.scientist.com>
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
 *
 * This file contains definitions for Drag and Drop functions in
 * gst-editor
 */

#ifndef __GSTE_DND_H__
#define __GSTE_DND_H__

typedef enum
{
  GSTE_DND_TYPE_NONE	= 0,
  GSTE_DND_TYPE_DIALOG	= 1,

  GST_DND_TYPE_LAST
} GsteDndType;

#define GSTE_TARGET_DIALOG \
        { "application/x-gste-dialog", GTK_TARGET_SAME_APP, GSTE_DND_TYPE_DIALOG }

#endif

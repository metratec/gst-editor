/* hacked for use in gstreamer by wingo@pobox.com */

/* GdkPixbuf library - GdkPixdata - functions for inlined pixbuf handling
 * Copyright (C) 1999, 2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __PIXDATA_H__
#define __PIXDATA_H__

#include        <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS
    GString * pixdata_to_csource (GdkPixdata * pixdata,
    const gchar * name, GdkPixdataDumpType dump_type);


G_END_DECLS
#endif /* __PIXDATA_H__ */

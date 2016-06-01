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
 */

#ifndef __GSTE_DEBUG_H__
#define __GSTE_DEBUG_H__

#include <gst/gst.h>

GST_DEBUG_CATEGORY_EXTERN (gste_debug_cat);

#ifdef G_HAVE_ISO_VARARGS
#define EDITOR_ERROR(...) 	GST_CAT_ERROR(gste_debug_cat, ##args)
#define EDITOR_WARNING(...)	GST_CAT_WARNING(gste_debug_cat, __VA_ARGS__)
#define EDITOR_INFO(...) 	GST_CAT_INFO(gste_debug_cat,  __VA_ARGS__)
#define EDITOR_DEBUG(...) 	GST_CAT_DEBUG(gste_debug_cat, __VA_ARGS__)
#define EDITOR_LOG(...) 	GST_CAT_LOG(gste_debug_cat, __VA_ARGS__)
#elif defined(G_HAVE_GNUC_VARARGS)
#define EDITOR_ERROR(args...) 	GST_CAT_ERROR(gste_debug_cat, ##args)
#define EDITOR_WARNING(args...)	GST_CAT_WARNING(gste_debug_cat, ##args)
#define EDITOR_INFO(args...) 	GST_CAT_INFO(gste_debug_cat, ##args)
#define EDITOR_DEBUG(args...) 	GST_CAT_DEBUG(gste_debug_cat, ##args)
#define EDITOR_LOG(args...) 	GST_CAT_LOG(gste_debug_cat, ##args)
#elif defined(_MSC_VER)
void EDITOR_ERROR(char *msg, ...);
void EDITOR_DEBUG(char *msg, ...);
void EDITOR_LOG(char *msg, ...);


#endif

#endif /* __GSTE_DEBUG_H__ */

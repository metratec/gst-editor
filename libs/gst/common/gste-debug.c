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
 * Add a category to the GStreamer debug framework for Gst Editor
 * output
 */  
    
#include <gst/gst.h>
    GST_DEBUG_CATEGORY (gste_debug_cat);
void 
gste_debug_init (void) 
{
  GST_DEBUG_CATEGORY_INIT (gste_debug_cat, "GSTE_EDITOR", 0,
      "GStreamer Editor messages");
} 

#ifdef _MSC_VER
#include <stdarg.h>
    void
EDITOR_ERROR (char *format, ...) 
{
  va_list varargs;
  va_start (varargs, format);
  GST_CAT_LEVEL_LOG_valist (gste_debug_cat, GST_LEVEL_ERROR, NULL, format,
      varargs);
  va_end (varargs);
} void
EDITOR_DEBUG (char *format, ...) 
{
  va_list varargs;
  va_start (varargs, format);
  GST_CAT_LEVEL_LOG_valist (gste_debug_cat, GST_LEVEL_DEBUG, NULL, format,
      varargs);
  va_end (varargs);
} void
EDITOR_LOG (char *format, ...) 
{
  va_list varargs;
  va_start (varargs, format);
  GST_CAT_LEVEL_LOG_valist (gste_debug_cat, GST_LEVEL_LOG, NULL, format,
      varargs);
  va_end (varargs);
} 
#endif  /*  */

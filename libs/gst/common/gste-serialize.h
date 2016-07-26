/* gst-editor
 * Copyright (C) <2016> Robin Haberkorn <haberkorn@metratec.com>
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

#ifndef __GSTE_SERIALIZE_H__
#define __GSTE_SERIALIZE_H__

#include <glib.h>

#include <gst/gst.h>

typedef void (*GsteSerializeAppendCallback) (const gchar * str,
    gpointer user_data);
typedef void (*GsteSerializeObjectSavedCallback) (GstObject * gstobject,
    gpointer user_data);

typedef enum _GsteSerializeFlags {
  /**
   * @private
   * Whether to output the next space or not.
   */
  GSTE_SERIALIZE_NEED_SPACE            = (1 << 0),
  /**
   * Enables verbose output, including default properties and
   * fully-qualified links.
   * Verbose serializations should still be valid.
   */
  GSTE_SERIALIZE_VERBOSE               = (1 << 1),
  /**
   * Whether all elements derived from GstBin should be serialized
   * as bins. This may break factory-created elements (e.g. autovideosink)
   * but help with custom GstBin subclasses.
   */
  GSTE_SERIALIZE_ALL_BINS              = (1 << 2),
  /**
   * Can be used to serialize all GstPipelines like GstBins.
   * Unlike GSTE_SERIALIZE_ALL_BINS, this has no functional disadvantages
   * and allows properties to be preserved even on the top-level pipeline.
   */
  GSTE_SERIALIZE_PIPELINES_AS_BINS     = (1 << 3),
  /**
   * Whether `capsfilter` elements are always serialized like normal elements.
   * This has no functional disadvantages and may help if the properties
   * of the capsfilter element are significant.
   * On the other hand, because of GstCapsFilter elements being curiously
   * ordered in the containing GstBin, the result will be much less readable.
   */
  GSTE_SERIALIZE_CAPSFILTER_AS_ELEMENT = (1 << 4)
} GsteSerializeFlags;

void gste_serialize_save (GstObject * object, GsteSerializeFlags flags,
    GsteSerializeAppendCallback append,
    GsteSerializeObjectSavedCallback object_saved,
    gpointer user_data);

#endif /* __GSTE_SERIALIZE_H__ */

/* hacked for use in gstreamer by wingo@pobox.com -- see bug XXXXX */

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

#include <gdk-pixbuf/gdk-pixdata.h>
#include "gdk-pixdata.h"

#include <string.h>

#define APPEND g_string_append_printf

/* --- functions --- */
static guint
pixdata_get_length (const GdkPixdata * pixdata)
{
  guint bpp, length;

  if ((pixdata->pixdata_type & GDK_PIXDATA_COLOR_TYPE_MASK) ==
      GDK_PIXDATA_COLOR_TYPE_RGB)
    bpp = 3;
  else if ((pixdata->pixdata_type & GDK_PIXDATA_COLOR_TYPE_MASK) ==
      GDK_PIXDATA_COLOR_TYPE_RGBA)
    bpp = 4;
  else
    return 0;			/* invalid format */
  switch (pixdata->pixdata_type & GDK_PIXDATA_ENCODING_MASK) {
      guint8 *rle_buffer;
      guint max_length;

    case GDK_PIXDATA_ENCODING_RAW:
      length = pixdata->rowstride * pixdata->height;
      break;
    case GDK_PIXDATA_ENCODING_RLE:
      /* need an RLE walk to determine size */
      max_length = pixdata->rowstride * pixdata->height;
      rle_buffer = pixdata->pixel_data;
      length = 0;
      while (length < max_length) {
	guint chunk_length = *(rle_buffer++);

	if (chunk_length & 128) {
	  chunk_length = chunk_length - 128;
	  if (!chunk_length)	/* RLE data corrupted */
	    return 0;
	  length += chunk_length * bpp;
	  rle_buffer += bpp;
	} else {
	  if (!chunk_length)	/* RLE data corrupted */
	    return 0;
	  chunk_length *= bpp;
	  length += chunk_length;
	  rle_buffer += chunk_length;
	}
      }
      length = rle_buffer - pixdata->pixel_data;
      break;
    default:
      length = 0;
      break;
  }
  return length;
}

typedef struct
{
  /* config */
  gboolean dump_stream;
  gboolean dump_struct;
  gboolean dump_macros;
  gboolean dump_gtypes;
  gboolean dump_rle_decoder;
  const gchar *static_prefix;
  const gchar *const_prefix;
  /* runtime */
  GString *gstring;
  guint pos;
  gboolean pad;
} CSourceData;

static inline void
save_uchar (CSourceData * cdata, guint8 d)
{
  GString *gstring = cdata->gstring;

  if (cdata->pos > 70) {
    if (cdata->dump_struct || cdata->dump_stream) {
      g_string_append (gstring, "\"\n  \"");
      cdata->pos = 3;
      cdata->pad = FALSE;
    }
    if (cdata->dump_macros) {
      g_string_append (gstring, "\" \\\n  \"");
      cdata->pos = 3;
      cdata->pad = FALSE;
    }
  }
  if (d < 33 || d > 126 || d == '?') {
    APPEND (gstring, "\\%o", d);
    cdata->pos += 1 + 1 + (d > 7) + (d > 63);
    cdata->pad = d < 64;
    return;
  }
  if (d == '\\') {
    g_string_append (gstring, "\\\\");
    cdata->pos += 2;
  } else if (d == '"') {
    g_string_append (gstring, "\\\"");
    cdata->pos += 2;
  } else if (cdata->pad && d >= '0' && d <= '9') {
    g_string_append (gstring, "\"\"");
    g_string_append_c (gstring, d);
    cdata->pos += 3;
  } else {
    g_string_append_c (gstring, d);
    cdata->pos += 1;
  }
  cdata->pad = FALSE;
  return;
}

static inline void
save_rle_decoder (GString * gstring,
    const gchar * macro_name,
    const gchar * s_uint, const gchar * s_uint_8, guint n_ch)
{
  APPEND (gstring,
      "#define %s_RUN_LENGTH_DECODE(image_buf, rle_data, size, bpp) do \\\n",
      macro_name);
  APPEND (gstring, "{ %s __bpp; %s *__ip; const %s *__il, *__rd; \\\n", s_uint,
      s_uint_8, s_uint_8);
  APPEND (gstring,
      "  __bpp = (bpp); __ip = (image_buf); __il = __ip + (size) * __bpp; \\\n");

  APPEND (gstring, "  __rd = (rle_data); if (__bpp > 3) { /* RGBA */ \\\n");

  APPEND (gstring, "    while (__ip < __il) { %s __l = *(__rd++); \\\n",
      s_uint);
  APPEND (gstring, "      if (__l & 128) { __l = __l - 128; \\\n");
  APPEND (gstring,
      "        do { memcpy (__ip, __rd, 4); __ip += 4; } while (--__l); __rd += 4; \\\n");
  APPEND (gstring, "      } else { __l *= 4; memcpy (__ip, __rd, __l); \\\n");
  APPEND (gstring, "               __ip += __l; __rd += __l; } } \\\n");

  APPEND (gstring, "  } else { /* RGB */ \\\n");

  APPEND (gstring, "    while (__ip < __il) { %s __l = *(__rd++); \\\n",
      s_uint);
  APPEND (gstring, "      if (__l & 128) { __l = __l - 128; \\\n");
  APPEND (gstring,
      "        do { memcpy (__ip, __rd, 3); __ip += 3; } while (--__l); __rd += 3; \\\n");
  APPEND (gstring, "      } else { __l *= 3; memcpy (__ip, __rd, __l); \\\n");
  APPEND (gstring, "               __ip += __l; __rd += __l; } } \\\n");

  APPEND (gstring, "  } } while (0)\n");
}

/**
 * gdk_pixdata_to_csource:
 * @pixdata: a #GdkPixdata to convert to C source.
 * @name: used for naming generated data structures or macros.
 * @dump_type: a #GdkPixdataDumpType determining the kind of C
 *   source to be generated.
 *
 * Generates C source code suitable for compiling images directly 
 * into programs. 
 *
 * GTK+ ships with a program called <command>gdk-pixbuf-csource</command> 
 * which offers a cmdline interface to this functions.
 *
 * Returns: a newly-allocated string containing the C source form
 *   of @pixdata.
 **/
GString *
pixdata_to_csource (GdkPixdata * pixdata,
    const gchar * name, GdkPixdataDumpType dump_type)
{
  CSourceData cdata = { 0, };
  gchar *s_uint_8, *s_uint_32, *s_uint, *s_char, *s_null;
  guint bpp, width, height, rowstride;
  gboolean rle_encoded;
  gchar *macro_name;
  guint8 *img_buffer, *img_buffer_end, *stream = NULL;
  guint stream_length;
  GString *gstring;

  /* check args passing */
  g_return_val_if_fail (pixdata != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  /* check pixdata contents */
  g_return_val_if_fail (pixdata->magic == GDK_PIXBUF_MAGIC_NUMBER, NULL);
  g_return_val_if_fail (pixdata->width > 0, NULL);
  g_return_val_if_fail (pixdata->height > 0, NULL);
  g_return_val_if_fail (pixdata->rowstride >= pixdata->width, NULL);
  g_return_val_if_fail ((pixdata->pixdata_type & GDK_PIXDATA_COLOR_TYPE_MASK) ==
      GDK_PIXDATA_COLOR_TYPE_RGB
      || (pixdata->pixdata_type & GDK_PIXDATA_COLOR_TYPE_MASK) ==
      GDK_PIXDATA_COLOR_TYPE_RGBA, NULL);
  g_return_val_if_fail ((pixdata->
	  pixdata_type & GDK_PIXDATA_SAMPLE_WIDTH_MASK) ==
      GDK_PIXDATA_SAMPLE_WIDTH_8, NULL);
  g_return_val_if_fail ((pixdata->pixdata_type & GDK_PIXDATA_ENCODING_MASK) ==
      GDK_PIXDATA_ENCODING_RAW
      || (pixdata->pixdata_type & GDK_PIXDATA_ENCODING_MASK) ==
      GDK_PIXDATA_ENCODING_RLE, NULL);
  g_return_val_if_fail (pixdata->pixel_data != NULL, NULL);

  img_buffer = pixdata->pixel_data;
  if (pixdata->length < 1)
    img_buffer_end = img_buffer + pixdata_get_length (pixdata);
  else
    img_buffer_end = img_buffer + pixdata->length - GDK_PIXDATA_HEADER_LENGTH;
  g_return_val_if_fail (img_buffer < img_buffer_end, NULL);

  bpp =
      (pixdata->pixdata_type & GDK_PIXDATA_COLOR_TYPE_MASK) ==
      GDK_PIXDATA_COLOR_TYPE_RGB ? 3 : 4;
  width = pixdata->width;
  height = pixdata->height;
  rowstride = pixdata->rowstride;
  rle_encoded = (pixdata->pixdata_type & GDK_PIXDATA_ENCODING_RLE) > 0;
  macro_name = g_ascii_strup (name, -1);

  cdata.dump_macros = (dump_type & GDK_PIXDATA_DUMP_MACROS) > 0;
  cdata.dump_struct = (dump_type & GDK_PIXDATA_DUMP_PIXDATA_STRUCT) > 0;
  cdata.dump_stream = !cdata.dump_macros && !cdata.dump_struct;
  g_return_val_if_fail (cdata.dump_macros + cdata.dump_struct +
      cdata.dump_stream == 1, NULL);

  cdata.dump_gtypes = (dump_type & GDK_PIXDATA_DUMP_CTYPES) == 0;
  cdata.dump_rle_decoder = (dump_type & GDK_PIXDATA_DUMP_RLE_DECODER) > 0;
  cdata.static_prefix = (dump_type & GDK_PIXDATA_DUMP_STATIC) ? "static " : "";
  cdata.const_prefix = (dump_type & GDK_PIXDATA_DUMP_CONST) ? "const " : "";
  gstring = g_string_new ("");
  cdata.gstring = gstring;

  if (!cdata.dump_macros && cdata.dump_gtypes) {
    s_uint_8 = "guint8 ";
    s_uint_32 = "guint32";
    s_uint = "guint  ";
    s_char = "gchar  ";
    s_null = "NULL";
  } else if (!cdata.dump_macros) {
    s_uint_8 = "unsigned char";
    s_uint_32 = "unsigned int ";
    s_uint = "unsigned int ";
    s_char = "char         ";
    s_null = "(char*) 0";
  } else if (cdata.dump_macros && cdata.dump_gtypes) {
    s_uint_8 = "guint8";
    s_uint_32 = "guint32";
    s_uint = "guint";
    s_char = "gchar";
    s_null = "NULL";
  } else {			/* cdata.dump_macros && !cdata.dump_gtypes */

    s_uint_8 = "unsigned char";
    s_uint_32 = "unsigned int";
    s_uint = "unsigned int";
    s_char = "char";
    s_null = "(char*) 0";
  }

  /* initial comment
   */
  APPEND (gstring,
      "/* GdkPixbuf %s C-Source image dump %s*/\n\n",
      bpp > 3 ? "RGBA" : "RGB",
      rle_encoded ? "1-byte-run-length-encoded " : "");

  /* dump RLE decoder for structures
   */
  if (cdata.dump_rle_decoder && cdata.dump_struct)
    save_rle_decoder (gstring,
	macro_name,
	cdata.dump_gtypes ? "guint" : "unsigned int",
	cdata.dump_gtypes ? "guint8" : "unsigned char", bpp);

  /* format & size blurbs
   */
  if (cdata.dump_macros) {
    APPEND (gstring, "#define %s_ROWSTRIDE (%u)\n", macro_name, rowstride);
    APPEND (gstring, "#define %s_WIDTH (%u)\n", macro_name, width);
    APPEND (gstring, "#define %s_HEIGHT (%u)\n", macro_name, height);
    APPEND (gstring, "#define %s_BYTES_PER_PIXEL (%u) /* 3:RGB, 4:RGBA */\n",
	macro_name, bpp);
  }
  if (cdata.dump_struct) {
    APPEND (gstring, "%s%sGdkPixdata %s = {\n",
	cdata.static_prefix, cdata.const_prefix, name);
    APPEND (gstring, "  0x%x, /* Pixbuf magic: 'GdkP' */\n",
	GDK_PIXBUF_MAGIC_NUMBER);
    APPEND (gstring, "  %u + %lu, /* header length + pixel_data length */\n",
	GDK_PIXDATA_HEADER_LENGTH,
	rle_encoded ? (glong) (img_buffer_end -
	    img_buffer) : (glong) rowstride * height);
    APPEND (gstring, "  0x%x, /* pixdata_type */\n", pixdata->pixdata_type);
    APPEND (gstring, "  %u, /* rowstride */\n", rowstride);
    APPEND (gstring, "  %u, /* width */\n", width);
    APPEND (gstring, "  %u, /* height */\n", height);
    APPEND (gstring, "  /* pixel_data: */\n");
  }
  if (cdata.dump_stream) {
    guint pix_length = img_buffer_end - img_buffer;

    stream = gdk_pixdata_serialize (pixdata, &stream_length);
    img_buffer = stream;
    img_buffer_end = stream + stream_length;

    APPEND (gstring, "%s%s%s %s[] = \n",
	cdata.static_prefix, cdata.const_prefix,
	cdata.dump_gtypes ? "guint8" : "unsigned char", name);
    APPEND (gstring, "{ \"\"\n  /* Pixbuf magic (0x%x) */\n  \"",
	GDK_PIXBUF_MAGIC_NUMBER);
    cdata.pos = 3;
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    APPEND (gstring, "\"\n  /* length: header (%u) + pixel_data (%u) */\n  \"",
	GDK_PIXDATA_HEADER_LENGTH,
	rle_encoded ? pix_length : rowstride * height);
    cdata.pos = 3;
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    APPEND (gstring, "\"\n  /* pixdata_type (0x%x) */\n  \"",
	pixdata->pixdata_type);
    cdata.pos = 3;
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    APPEND (gstring, "\"\n  /* rowstride (%u) */\n  \"", rowstride);
    cdata.pos = 3;
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    APPEND (gstring, "\"\n  /* width (%u) */\n  \"", width);
    cdata.pos = 3;
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    APPEND (gstring, "\"\n  /* height (%u) */\n  \"", height);
    cdata.pos = 3;
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    save_uchar (&cdata, *img_buffer++);
    APPEND (gstring, "\"\n  /* pixel_data: */\n");
  }

  /* pixel_data intro
   */
  if (cdata.dump_macros) {
    APPEND (gstring, "#define %s_%sPIXEL_DATA ((%s*) \\\n",
	macro_name, rle_encoded ? "RLE_" : "", s_uint_8);
    APPEND (gstring, "  \"");
    cdata.pos = 2;
  }
  if (cdata.dump_struct) {
    APPEND (gstring, "  \"");
    cdata.pos = 3;
  }
  if (cdata.dump_stream) {
    APPEND (gstring, "  \"");
    cdata.pos = 3;
  }

  /* pixel_data
   */
  do
    save_uchar (&cdata, *img_buffer++);
  while (img_buffer < img_buffer_end);

  /* pixel_data trailer
   */
  if (cdata.dump_macros)
    APPEND (gstring, "\")\n\n");
  if (cdata.dump_struct)
    APPEND (gstring, "\",\n};\n\n");
  if (cdata.dump_stream)
    APPEND (gstring, "\"};\n\n");

  /* dump RLE decoder for macros
   */
  if (cdata.dump_rle_decoder && cdata.dump_macros)
    save_rle_decoder (gstring,
	macro_name,
	cdata.dump_gtypes ? "guint" : "unsigned int",
	cdata.dump_gtypes ? "guint8" : "unsigned char", bpp);

  /* cleanup
   */
  g_free (stream);
  g_free (macro_name);

  return gstring;
}

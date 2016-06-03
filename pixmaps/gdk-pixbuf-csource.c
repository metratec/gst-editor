/* Gdk-Pixbuf-CSource - GdkPixbuf based image CSource generator
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
#include "config.h"

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gdk-pixdata.h"

/* --- defines --- */
#undef	G_LOG_DOMAIN
#define	G_LOG_DOMAIN	"Gdk-Pixbuf-CSource"
#define PRG_NAME        "gdk-pixbuf-csource"
#define PKG_NAME        "Gtk+"
#define PKG_HTTP_HOME   "http://www.gtk.org"


/* --- prototypes --- */
static void parse_args (gint * argc_p, gchar *** argv_p);
static void print_blurb (FILE * bout, gboolean print_help);


/* --- variables --- */
static guint gen_type = GDK_PIXDATA_DUMP_PIXDATA_STREAM;
static guint gen_ctype =
    GDK_PIXDATA_DUMP_GTYPES | GDK_PIXDATA_DUMP_STATIC | GDK_PIXDATA_DUMP_CONST;
static gboolean use_rle = TRUE;
static gboolean with_decoder = FALSE;
static gchar *image_name = "my_pixbuf";
static gboolean build_list = FALSE;


/* --- functions --- */
static void
print_csource (FILE * f_out, GdkPixbuf * pixbuf)
{
  GdkPixdata pixdata;
  gpointer free_me;
  GString *gstring;

  free_me = gdk_pixdata_from_pixbuf (&pixdata, pixbuf, use_rle);
  gstring = pixdata_to_csource (&pixdata, image_name,
      gen_type | gen_ctype | (with_decoder ? GDK_PIXDATA_DUMP_RLE_DECODER : 0));

  fprintf (f_out, "%s\n", gstring->str);

  g_free (free_me);
}

int
main (int argc, char *argv[])
{
  GdkPixbuf *pixbuf;
  GError *error = NULL;

  /* parse args and do fast exits */
  parse_args (&argc, &argv);

  if (!build_list) {
    if (argc != 2) {
      print_blurb (stderr, TRUE);
      return 1;
    }

    pixbuf = gdk_pixbuf_new_from_file (argv[1], &error);
    if (!pixbuf) {
      fprintf (stderr, "failed to load \"%s\": %s\n", argv[1], error->message);
      g_error_free (error);
      return 1;
    }

    print_csource (stdout, pixbuf);
    g_object_unref (pixbuf);
  } else {			/* parse name, file pairs */

    gchar **p = argv + 1;
    guint j = argc - 1;
    gboolean toggle = FALSE;

    while (j--) {
      if (!toggle)
	image_name = *p++;
      else {
	pixbuf = gdk_pixbuf_new_from_file (*p, &error);
	if (!pixbuf) {
	  fprintf (stderr, "failed to load \"%s\": %s\n", *p, error->message);
	  g_error_free (error);
	  return 1;
	}
	print_csource (stdout, pixbuf);
	g_object_unref (pixbuf);
	p++;
      }
      toggle = !toggle;
    }
  }

  return 0;
}

static void
parse_args (gint * argc_p, gchar *** argv_p)
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  guint i, e;

  for (i = 1; i < argc; i++) {
    if (strcmp ("--macros", argv[i]) == 0) {
      gen_type = GDK_PIXDATA_DUMP_MACROS;
      argv[i] = NULL;
    } else if (strcmp ("--struct", argv[i]) == 0) {
      gen_type = GDK_PIXDATA_DUMP_PIXDATA_STRUCT;
      argv[i] = NULL;
    } else if (strcmp ("--stream", argv[i]) == 0) {
      gen_type = GDK_PIXDATA_DUMP_PIXDATA_STREAM;
      argv[i] = NULL;
    } else if (strcmp ("--rle", argv[i]) == 0) {
      use_rle = TRUE;
      argv[i] = NULL;
    } else if (strcmp ("--raw", argv[i]) == 0) {
      use_rle = FALSE;
      argv[i] = NULL;
    } else if (strcmp ("--extern", argv[i]) == 0) {
      gen_ctype &= ~GDK_PIXDATA_DUMP_STATIC;
      argv[i] = NULL;
    } else if (strcmp ("--static", argv[i]) == 0) {
      gen_ctype |= GDK_PIXDATA_DUMP_STATIC;
      argv[i] = NULL;
    } else if (strcmp ("--decoder", argv[i]) == 0) {
      with_decoder = TRUE;
      argv[i] = NULL;
    } else if ((strcmp ("--name", argv[i]) == 0) ||
	(strncmp ("--name=", argv[i], 7) == 0)) {
      gchar *equal = argv[i] + 6;

      if (*equal == '=')
	image_name = g_strdup (equal + 1);
      else if (i + 1 < argc) {
	image_name = g_strdup (argv[i + 1]);
	argv[i] = NULL;
	i += 1;
      }
      argv[i] = NULL;
    } else if (strcmp ("--build-list", argv[i]) == 0) {
      build_list = TRUE;
      argv[i] = NULL;
    } else if (strcmp ("-h", argv[i]) == 0 || strcmp ("--help", argv[i]) == 0) {
      print_blurb (stderr, TRUE);
      argv[i] = NULL;
      exit (0);
    } else if (strcmp ("-v", argv[i]) == 0 ||
	strcmp ("--version", argv[i]) == 0) {
      print_blurb (stderr, FALSE);
      argv[i] = NULL;
      exit (0);
    } else if (strcmp (argv[i], "--g-fatal-warnings") == 0) {
      GLogLevelFlags fatal_mask;

      fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
      fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
      g_log_set_always_fatal (fatal_mask);

      argv[i] = NULL;
    }
  }

  e = 0;
  for (i = 1; i < argc; i++) {
    if (e) {
      if (argv[i]) {
	argv[e++] = argv[i];
	argv[i] = NULL;
      }
    } else if (!argv[i])
      e = i;
  }
  if (e)
    *argc_p = e;
}

static void
print_blurb (FILE * bout, gboolean print_help)
{
  if (!print_help) {
    fprintf (bout, "%s version ", PRG_NAME);
    fprintf (bout, "%u.%u.%u", GTK_MAJOR_VERSION, GTK_MINOR_VERSION,
	GTK_MICRO_VERSION);
    fprintf (bout, "\n");
    fprintf (bout, "%s comes with ABSOLUTELY NO WARRANTY.\n", PRG_NAME);
    fprintf (bout, "You may redistribute copies of %s under the terms of\n",
	PRG_NAME);
    fprintf (bout,
	"the GNU Lesser General Public License which can be found in the\n");
    fprintf (bout, "%s source package. Sources, examples and contact\n",
	PKG_NAME);
    fprintf (bout, "information are available at %s\n", PKG_HTTP_HOME);
  } else {
    fprintf (bout, "Usage: %s [options] [image]\n", PRG_NAME);
    fprintf (bout, "       %s [options] --build-list [[name image]...]\n",
	PRG_NAME);
    fprintf (bout,
	"  --stream                   generate pixbuf data stream\n");
    fprintf (bout,
	"  --struct                   generate GdkPixdata structure\n");
    fprintf (bout,
	"  --macros                   generate image size/pixel macros\n");
    fprintf (bout,
	"  --rle                      use one byte run-length-encoding\n");
    fprintf (bout,
	"  --raw                      provide raw image data copy\n");
    fprintf (bout, "  --extern                   generate extern symbols\n");
    fprintf (bout, "  --static                   generate static symbols\n");
    fprintf (bout, "  --decoder                  provide rle decoder\n");
    fprintf (bout, "  --name=identifier          C macro/variable name\n");
    fprintf (bout, "  --build-list               parse (name, image) pairs\n");
    fprintf (bout, "  -h, --help                 show this help message\n");
    fprintf (bout, "  -v, --version              print version informations\n");
    fprintf (bout,
	"  --g-fatal-warnings         make warnings fatal (abort)\n");
  }
}

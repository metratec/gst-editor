noinst_LTLIBRARIES = libgste-common.la

built_header_make = gste-marshal.h
built_source_make = gste-marshal.c

libgste_common_la_SOURCES = \
    gste-debug.c \
    gste-common.c
nodist_libgste_common_la_SOURCES = $(built_source_make)

libgste_common_la_CFLAGS = -DDATADIR="\"$(pkgdatadir)/\"" $(GST_EDITOR_CFLAGS)
libgste_common_la_LDFLAGS = $(GST_EDITOR_LIBS)

noinst_HEADERS = \
  gste-debug.h gste-dnd.h gste-dock.h \
  gste-common-priv.h gste-common.h

BUILT_SOURCES = $(built_source_make) $(built_header_make)
CLEANFILES = $(built_source_make) $(built_header_make)

EXTRA_DIST = gste-marshal.list

gste-marshal.h: gste-marshal.list
	glib-genmarshal --header --prefix=gst_editor_marshal $^ > gste-marshal.h.tmp
	mv gste-marshal.h.tmp gste-marshal.h
                                                                                
gste-marshal.c: gste-marshal.list
	echo "#include \"gste-marshal.h\"" >> gste-marshal.c.tmp
	glib-genmarshal --body --prefix=gst_editor_marshal $^ >> gste-marshal.c.tmp
	mv gste-marshal.c.tmp gste-marshal.c

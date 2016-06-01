pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gst-editor.png gst-inspect.png gst-launch.png

gladedir = $(datadir)/gsteditor
glade_DATA = gst-editor.png 

noinst_PROGRAMS = gdk-pixbuf-csource

gdk_pixbuf_csource_SOURCES = gdk-pixbuf-csource.c gdk-pixdata.h gdk-pixdata.c
gdk_pixbuf_csource_CFLAGS = $(GDK_PIXBUF_CFLAGS) $(GTK_CFLAGS)
gdk_pixbuf_csource_LDADD = $(GDK_PIXBUF_LIBS) $(GTK_LIBS)

PIXMAPS = gst-editor.png off.png on.png pause.png play.png

EXTRA_DIST = $(PIXMAPS) pixmaps.h copyright.txt $(pixmap_DATA) $(glade_DATA)

noinst_HEADERS = pixmaps.h
BUILT_SOURCES = pixmaps.h

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = $(BUILT_SOURCES)

pixmaps.h: gdk-pixbuf-csource $(PIXMAPS)
	-rm -f pixmaps.h pixmaps-tmp
	for i in $(PIXMAPS); do \
	   ./gdk-pixbuf-csource --raw --build-list \
	      `echo $$i | sed -e 's/\.png/_stock_image/' | sed -e 's/-/_/g'` \
	      $(srcdir)/$$i >> pixmaps-tmp; \
	done
	cat $(srcdir)/copyright.txt pixmaps-tmp > pixmaps.h
	-rm -f pixmaps-tmp

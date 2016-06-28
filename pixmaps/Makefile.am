pixmapdir = $(datadir)/pixmaps
dist_pixmap_DATA = gst-editor.png gst-inspect.png gst-launch.png

dist_pkgdata_DATA = gst-editor.png

PIXMAPS = gst-editor.png off.png on.png pause.png play.png

EXTRA_DIST = $(PIXMAPS) pixmaps.h copyright.txt

noinst_HEADERS = pixmaps.h
BUILT_SOURCES = pixmaps.h

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = $(BUILT_SOURCES)

pixmaps.h: $(PIXMAPS)
	-rm -f pixmaps.h pixmaps-tmp
	for i in $(PIXMAPS); do \
	   $(GDK_PIXBUF_CSOURCE) --raw --build-list \
	      `echo $$i | sed -e 's/\.png/_stock_image/' | sed -e 's/-/_/g'` \
	      $(srcdir)/$$i >> pixmaps-tmp; \
	done
	cat $(srcdir)/copyright.txt pixmaps-tmp > pixmaps.h
	-rm -f pixmaps-tmp

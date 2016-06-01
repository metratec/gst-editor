lib_LTLIBRARIES = libgsteditor.la

libgsteditor_la_SOURCES =	\
	gsteditor.c		\
	gsteditorbin.c		\
	gsteditorcanvas.c	\
	gsteditorelement.c	\
	gsteditorimage.c	\
	gsteditoritem.c		\
	gsteditorlink.c	\
	gsteditorpad.c		\
	gsteditorpalette.c	\
	gsteditorpopup.c	\
	gsteditorproperty.c	\
	gsteditorstatusbar.c \
	gst-helper.c		\
	stockicons.c

libgsteditor_la_CFLAGS = -DDATADIR="\"$(pkgdatadir)/\"" $(GST_EDITOR_CFLAGS)
libgsteditor_la_LDFLAGS = $(GST_EDITOR_LIBS) \
	$(top_builddir)/libs/gst/common/libgste-common.la \
	$(top_builddir)/libs/gst/element-ui/libgstelementui.la \
	$(top_builddir)/libs/gst/element-browser/libgstelementbrowser.la \
	$(top_builddir)/libs/gst/debug-ui/libgstdebugui.la

noinst_HEADERS =                \
        gsteditorproperty.h     \
	gsteditorpopup.h	\
        gsteditorpalette.h      \
        gsteditorimage.h        \
	gst-helper.h		\
	stockicons.h

libgsteditorincludedir = $(includedir)/@PACKAGE@-@VERSION@/gst/editor
libgsteditorinclude_HEADERS = editor.h \
	gsteditor.h \
	gsteditorbin.h \
	gsteditorelement.h \
	gsteditoritem.h \
	gsteditorpad.h \
	gsteditorlink.h \
	gsteditorcanvas.h \
        gsteditorstatusbar.h

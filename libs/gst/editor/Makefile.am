lib_LTLIBRARIES = libgsteditor.la

libgsteditor_la_SOURCES =	\
	gsteditor.c		\
	gsteditorbin.c		\
	gsteditorcanvas.c	\
	gsteditorelement.c	\
	gsteditoritem.c		\
	gsteditorlink.c	\
	gsteditorpad.c		\
	gsteditorpalette.c	\
	gsteditorpopup.c	\
	gsteditorproperty.c	\
	gst-helper.c		\
	namedicons.c

libgsteditor_la_CFLAGS = -DDATADIR="\"$(pkgdatadir)/\"" $(GST_EDITOR_CFLAGS)
libgsteditor_la_LDFLAGS = $(GST_EDITOR_LIBS) \
	$(top_builddir)/libs/gst/common/libgste-common.la \
	$(top_builddir)/libs/gst/element-ui/libgstelementui.la \
	$(top_builddir)/libs/gst/element-browser/libgstelementbrowser.la \
	$(top_builddir)/libs/gst/debug-ui/libgstdebugui.la

noinst_HEADERS =                \
	gsteditorpopup.h	\
        gsteditorpalette.h      \
	gst-helper.h		\
	namedicons.h

libgsteditorincludedir = $(includedir)/@PACKAGE@-@GST_API_VERSION@/gst/editor
libgsteditorinclude_HEADERS = editor.h \
	gsteditor.h \
	gsteditorbin.h \
	gsteditorelement.h \
	gsteditoritem.h \
	gsteditorpad.h \
	gsteditorlink.h \
	gsteditorcanvas.h \
	gsteditorproperty.h

lib_LTLIBRARIES = libgstdebugui.la

libgstdebugui_la_SOURCES =	\
	debug-ui.c

libgstdebugui_la_CFLAGS = -DDATADIR="\"$(pkgdatadir)/\"" $(GST_EDITOR_CFLAGS)
libgstdebugui_la_LDFLAGS = $(GST_EDITOR_LIBS)

libgstdebuguiincludedir = $(includedir)/@PACKAGE@-@VERSION@/gst/debug-ui
libgstdebuguiinclude_HEADERS = debug-ui.h

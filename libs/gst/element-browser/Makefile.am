lib_LTLIBRARIES = libgstelementbrowser.la

libgstelementbrowser_la_SOURCES =	\
	browser.c			\
	caps-tree.c			\
	element-tree.c

libgstelementbrowser_la_CFLAGS = -DDATADIR="\"$(pkgdatadir)/\"" $(GST_EDITOR_CFLAGS)
libgstelementbrowser_la_LDFLAGS = $(GST_EDITOR_LIBS)

libgstelementbrowserincludedir = $(includedir)/@PACKAGE@-@VERSION@/gst/element-browser
libgstelementbrowserinclude_HEADERS = browser.h caps-tree.h element-tree.h

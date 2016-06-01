bin_PROGRAMS = gst-editor gst-launch-gui gst-inspect-gui

gst_editor_SOURCES = editor.c
gst_editor_LDFLAGS = $(top_builddir)/libs/gst/editor/libgsteditor.la $(GST_EDITOR_LIBS)
gst_editor_CFLAGS = $(GST_EDITOR_CFLAGS) -DDATADIR=\"$(gladedir)/\" -I$(top_srcdir)/libs

gst_launch_gui_LDADD = $(GST_EDITOR_LIBS)\
	 $(top_builddir)/libs/gst/common/libgste-common.la \
	 $(top_builddir)/libs/gst/element-ui/libgstelementui.la \
	 $(top_builddir)/libs/gst/debug-ui/libgstdebugui.la
gst_launch_gui_CFLAGS = $(GST_EDITOR_CFLAGS) -DPIXMAP_DIR=\"$(datadir)/pixmaps/\"

gst_inspect_gui_LDADD = $(GST_EDITOR_LIBS) \
	 $(top_builddir)/libs/gst/element-browser/libgstelementbrowser.la \
	 $(top_builddir)/libs/gst/debug-ui/libgstdebugui.la \
	 $(top_builddir)/libs/gst/common/libgste-common.la 
gst_inspect_gui_CFLAGS = $(GST_EDITOR_CFLAGS)

gladedir = $(datadir)/gsteditor
glade_DATA = editor.glade2 

man_MANS = gst-editor.1 gst-launch-gui.1 gst-inspect-gui.1

EXTRA_DIST = $(glade_DATA) $(man_MANS)

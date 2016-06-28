AC_PREREQ(2.65)

dnl This is the configure.ac file for the gst-editor module
dnl when going to/from release please set the nano (fourth number) right !
dnl releases only do Wall, cvs and prerelease does Werror too
AC_INIT([GstEditor], [1.2.4.0],
        [https://github.com/metratec/gst-editor/], [gsteditor])

AC_CONFIG_MACRO_DIR([common/m4])
AC_CONFIG_AUX_DIR([config])

dnl initialize automake
AM_INIT_AUTOMAKE
#AC_PROG_INTLTOOL

dnl define PACKAGE_VERSION_* variables
AS_VERSION

dnl check if this is a release version
AS_NANO(GST_CVS="no", GST_CVS="yes")

dnl can autoconf find the source ?
AC_CONFIG_SRCDIR([src/editor.c])

dnl define the output header for config
AC_CONFIG_HEADERS([config.h])

dnl AM_MAINTAINER_MODE only provides the option to configure to enable it
AM_MAINTAINER_MODE

dnl sets host_* variables
AC_CANONICAL_HOST

dnl set up gettext
dnl the version check needs to stay here because autopoint greps for it
AM_GNU_GETTEXT_VERSION([0.11.5])
AM_GNU_GETTEXT([external])
AG_GST_GETTEXT([gsteditor-$GST_MAJORMINOR])

dnl We disable static building for development, for time savings
dnl AM_DISABLE_STATIC
AS_LIBTOOL(GST_EDITOR, 0, 0, 0, yes)
AM_PROG_LIBTOOL

AC_PROG_INTLTOOL

dnl uninstalled GtkBuilder ui dir
AC_DEFINE_UNQUOTED(GTKBUILDER_UNINSTALLED_DIR,"`pwd`/src",[path to GtkBuilder ui dir])
AC_SUBST(GTKBUILDER_UNINSTALLED_DIR)

dnl Add parameters for aclocal
dnl (This must come after AM_INIT_AUTOMAKE, since it modifies ACLOCAL)
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4 -I common/m4")

AC_PROG_CC
AC_PROG_CC_C99
AC_ISC_POSIX

dnl AC_HEADER_STDC([])

dnl stuff found by autoscan
AC_CHECK_FUNCS([strstr])
AC_CHECK_HEADERS([unistd.h])
AC_C_CONST
AC_FUNC_STAT

dnl allow different autotools
AS_AUTOTOOLS_ALTERNATE

AC_CHECK_PROG(HAVE_SCROLLKEEPER, [scrollkeeper-preinstall], true, false)
`$HAVE_SCROLLKEEPER` || AC_MSG_NOTICE([Scrollkeeper not found, not installing help files.])

dnl get the pixbuf source creation script
dnl This is installed from GTK+ 2.0's gdk-pixbuf
AC_PATH_PROG(GDK_PIXBUF_CSOURCE, gdk-pixbuf-csource)
test -z "$GDK_PIXBUF_CSOURCE" && AC_MSG_ERROR([
*** You need the gdk-pixbuf-csource tool which is installed
*** from GTK+'s gdk-pixbuf.
])

AG_GST_DEBUGINFO

dnl modify pkg-config path
AC_ARG_WITH(pkg-config-path, 
   AC_HELP_STRING([--with-pkg-config-path],[colon-separated list of pkg-config(1) dirs]),
   [export PKG_CONFIG_PATH=${withval}])

dnl decide on error flags
AS_COMPILER_FLAG(-Wall, GST_WALL="yes", GST_WALL="no")

if test "x$GST_WALL" = "xyes"; then
   GST_ERROR="$GST_ERROR -Wall"

   if test "x$GST_CVS" = "xyes"; then
	AS_COMPILER_FLAG(-Werror,GST_ERROR="$GST_ERROR",GST_ERROR="$GST_ERROR")
   fi
fi
dnl check for appropriate GStreamer libs.
GSTREAMER_REQ="1.2.4"
AC_SUBST(GSTREAMER_REQ)

GSTREAMER_MAJORMINOR="1.0"
PKG_CHECK_MODULES(GST, gstreamer-$GSTREAMER_MAJORMINOR >= $GSTREAMER_REQ,,
                  [AC_MSG_ERROR([GStreamer $GSTREAMER_MAJORMINOR required but not found])])

PKG_CHECK_MODULES(GOOCANVAS, goocanvas-2.0 >= 2.0,,
   [AC_MSG_ERROR([GOOCANVAS 2.0 required but not found])])

PKG_CHECK_MODULES(GDK_PIXBUF, gdk-pixbuf-2.0,,
   [AC_MSG_ERROR([gdk-pixbuf 2.0 required but not found])])

PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.10.8,,
   [AC_MSG_ERROR([GTK 3.10.8 required but not found])])

AC_SUBST(GST_CFLAGS)
AC_SUBST(GST_LIBS)
AC_SUBST(GOOCANVAS_CFLAGS)
AC_SUBST(GOOCANVAS_LIBS)
AC_SUBST(GDK_PIXBUF_CFLAGS)
AC_SUBST(GDK_PIXBUF_LIBS)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

GST_EDITOR_CFLAGS="$GST_CFLAGS $GOOCANVAS_CFLAGS $GDK_PIXBUF_CFLAGS $GTK_CFLAGS  -I\$(top_srcdir) -I\$(top_srcdir)/libs -I\$(top_builddir)/libs $GST_ERROR"
GST_EDITOR_LIBS="$GST_LIBS $GOOCANVAS_LIBS $GDK_PIXBUF_LIBS $GTK_LIBS"

AC_SUBST(GST_EDITOR_CFLAGS)
AC_SUBST(GST_EDITOR_LIBS)

AS_AC_EXPAND(GST_EDITOR_DATA_DIR, $datadir/$PACKAGE)
AC_DEFINE_UNQUOTED(GST_EDITOR_DATA_DIR, "$GST_EDITOR_DATA_DIR",
                   [The application-specific data dir])

dnl translation stuff
dnl we explicitly specify gst-editor instead of $PACKAGE because intltool
dnl programs search for this variable
GETTEXT_PACKAGE=gst-editor
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", "gettext package name")

dnl define LOCALEDIR in config.h
AS_AC_EXPAND(LOCALEDIR, $datadir/locale)
AC_DEFINE_UNQUOTED([LOCALEDIR], "$LOCALEDIR",
                   [gettext locale dir])

ALL_LINGUAS="nl no"
AM_GLIB_GNU_GETTEXT

AM_CONDITIONAL(HAVE_SCROLLKEEPER,    $HAVE_SCROLLKEEPER)

AC_OUTPUT(
Makefile 
help/Makefile
help/C/Makefile
libs/Makefile 
libs/gst/Makefile 
libs/gst/common/Makefile
libs/gst/editor/Makefile 
libs/gst/element-ui/Makefile 
libs/gst/debug-ui/Makefile 
libs/gst/element-browser/Makefile 
src/Makefile
pixmaps/Makefile
gst-editor-libs.pc
gst-editor-libs-uninstalled.pc
po/Makefile.in
)

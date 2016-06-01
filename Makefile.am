if HAVE_SCROLLKEEPER
SCROLLKEEPER_DIR = help
else
SCROLLKEEPER_DIR =
endif

SUBDIRS = pixmaps $(SCROLLKEEPER_DIR) libs src po
DIST_SUBDIRS = pixmaps help libs src po

DESKTOP_IN_FILES = \
	gst-editor.desktop.in \
	gst-inspect.desktop.in \
	gst-launch.desktop.in
DESKTOP_FILES = $(DESKTOP_IN_FILES:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

MMdir = $(datadir)/applications
MM_DATA = $(DESKTOP_FILES)

EXTRA_DIST = \
	mkinstalldirs gst-editor.spec \
	intltool-extract.in     \
	intltool-merge.in       \
	intltool-update.in      \
	$(DESKTOP_FILES) $(DESKTOP_IN_FILES) \
	RELEASE HACKING TODO

INTLTOOL_BUILT = \
	intltool-extract \
	intltool-merge \
	intltool-update
                                                                                
DISTCLEANFILES = $(INTLTOOL_BUILT) $(DESKTOP_FILES)

# to do a release, run "make release"
# this will generate all archives and md5sums
                                                                                
# make bz2 as well
AUTOMAKE_OPTIONS = dist-bzip2
                                                                                
release: dist
	make @PACKAGE@-@VERSION@.tar.gz.md5
	make @PACKAGE@-@VERSION@.tar.bz2.md5
                                                                                
# generate md5 sum files
%.md5: %
	md5sum $< > $@
                                                                                
dist-hook:
	cp gst-editor.spec $(distdir)
	cp -r ./common $(distdir)
	rm -rf `find $(distdir)/common -name CVS`
                                                                                
rpm: dist
	rpm -ta $(distdir).tar.gz

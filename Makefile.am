## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = m4 src po help

EXTRA_DIST =		\
	COPYING-DOCS	\
	autogen.sh	\
	gmfsk.glade	\
	gmfsk.gladep	\
	gmfsk.spec	\
	gmfsk.spec.in	\
	xmldocs.make	\
	omf.make	\
	$(schema_DATA)

schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = gmfsk.schemas

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \
	    fi \
	  done \
	fi
	if test -d $(srcdir)/fonts; then \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/fonts/feldhell; \
	  for font in $(srcdir)/fonts/*; do \
	    if test -f $$font; then \
	      $(INSTALL_DATA) $$font $(DESTDIR)$(datadir)/fonts/feldhell; \
	    fi \
	  done \
	fi
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-install-rule $(top_builddir)/gmfsk.schemas

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi
	if test -d fonts; then \
	  mkdir $(distdir)/fonts; \
	  for font in fonts/*; do \
	    if test -f $$font; then \
	      cp -p $$font $(distdir)/fonts; \
	    fi \
	  done \
	fi

rpm:	Makefile
	make dist
	$(RPMBUILD) -ta $(PACKAGE)-$(VERSION).tar.gz
	rm $(PACKAGE)-$(VERSION).tar.gz


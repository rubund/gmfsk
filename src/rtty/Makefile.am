INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/misc

noinst_LIBRARIES = librtty.a

librtty_a_SOURCES = \
	baudot.c baudot.h			\
	rttypar.c rttypar.h			\
	rtty.c rtty.h				\
	rttyrx.c				\
	rttytx.c


INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/misc

noinst_LIBRARIES = libpsk31.a

libpsk31_a_SOURCES = \
	coeff.c coeff.h				\
	psk31.c psk31.h				\
	psk31rx.c				\
	psk31tx.c				\
	varicode.c varicode.h


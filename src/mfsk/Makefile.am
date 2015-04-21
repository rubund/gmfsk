INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/misc

noinst_LIBRARIES = libmfsk.a

libmfsk_a_SOURCES = \
	interleave.c interleave.h		\
	mfsk.c mfsk.h				\
	mfskrx.c				\
	mfsktx.c				\
	varicode.c varicode.h

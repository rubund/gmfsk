INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/misc

noinst_LIBRARIES = libmt63.a

libmt63_a_SOURCES = \
	alias_1k.dat alias_2k.dat alias_k5.dat	\
	dsp.cc dsp.h				\
	morse.dat				\
	mt63.cc mt63.h				\
	mt63intl.dat				\
	mt63main.cc				\
	symbol.dat

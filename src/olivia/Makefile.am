INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/misc

noinst_LIBRARIES = libolivia.a

libolivia_a_SOURCES =	\
	cmpx.h		\
	fft.h		\
	fht.h		\
	fifo.h		\
	gray.h		\
	lowpass3.h	\
	mfsk.h		\
	struc.h		\
	olivia.cc

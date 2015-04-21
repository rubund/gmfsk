INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

noinst_LIBRARIES = libsamplerate.a

libsamplerate_a_SOURCES =	\
	common.h		\
	fastest_coeffs.h	\
	float_cast.h		\
	samplerate.c		\
	samplerate.h		\
	src_sinc.c

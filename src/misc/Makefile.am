INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

noinst_PROGRAMS = genfilt
genfilt_SOURCES = genfilt.c

noinst_LIBRARIES = libmisc.a

libmisc_a_SOURCES = \
	cmplx.c cmplx.h				\
	misc.c misc.h				\
	delay.c delay.h				\
	fft.c fft.h				\
	fftfilt.c fftfilt.h			\
	filter-i386.h filter.c filter.h		\
	sfft.c sfft.h				\
	viterbi.c viterbi.h

genfilt_LDADD = -lm


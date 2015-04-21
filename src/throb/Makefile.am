INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/misc

noinst_LIBRARIES = libthrob.a

libthrob_a_SOURCES = \
	tab.c tab.h		\
	throb.c throb.h		\
	throbrx.c		\
	throbtx.c


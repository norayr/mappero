lib_LTLIBRARIES = libmappero.la

AM_CFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"mappero\" \
	-std=c99 -Wall -Wno-switch

LDADD = \
	-export-dynamic \
	-export-symbols-regex "^map.*" \
	-rpath $(libdir)

BUILT_SOURCES =

mappero_includedir = $(includedir)/mappero
mappero_include_HEADERS = \
	globals.h \
	path.h \
	router.h

libmappero_la_SOURCES = \
	path.c \
	router.c

libmappero_la_CFLAGS = \
	$(GCONF_CFLAGS) \
	$(GTK_CFLAGS)

libmappero_la_LIBADD = \
	$(GCONF_LIBS) \
	$(GTK_LIBS)

CLEANFILES = \
	$(BUILT_SOURCES)


lib_LTLIBRARIES = libmappero.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mappero.pc

AM_CFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"mappero\" \
	-std=c99 -Wall -Wno-switch

LDADD = \
	-export-dynamic \
	-export-symbols-regex "^map.*" \
	-rpath $(libdir)

nodist_libmappero_la_SOURCES = \
	map-signals-marshal.c \
	map-signals-marshal.h

BUILT_SOURCES = \
	$(nodist_libmappero_la_SOURCES)

mappero_includedir = $(includedir)/mappero-@MAP_API_VERSION@/mappero
mappero_include_HEADERS = \
	debug.h \
	error.h \
	globals.h \
	loader.h \
	path.h \
	router.h \
	viewer.h \
	util.h

libmappero_la_SOURCES = \
	debug.c \
	error.c \
	loader.c \
	path.c \
	router.c \
	viewer.c \
	util.c

libmappero_la_CFLAGS = \
	$(GCONF_CFLAGS) \
	$(GTK_CFLAGS)

libmappero_la_LIBADD = \
	$(GCONF_LIBS) \
	$(GTK_LIBS) \
	-lm -lrt

libmappero_la_LDFLAGS = \
	-version-info "@MAP_LT_CURRENT@":"@MAP_LT_REVISION@":"@MAP_LT_AGE@" \
	-Wl,--no-undefined

EXTRA_DIST = \
	map-signals-marshal.list

CLEANFILES = \
	$(BUILT_SOURCES)

%-signals-marshal.h: %-signals-marshal.list Makefile
	glib-genmarshal --header --prefix=_$(subst -,_,$(notdir $*))_marshal $< \
	  > $*-signals-marshal.h

%-signals-marshal.c: %-signals-marshal.list Makefile
	{ echo '#include "$(notdir $*)-signals-marshal.h"' && \
	  glib-genmarshal --body --prefix=_$(subst -,_,$(notdir $*))_marshal $<; } \
	    > $*-signals-marshal.c

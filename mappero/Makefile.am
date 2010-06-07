lib_LTLIBRARIES = libmappero.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mappero.pc

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
	gpx.h \
	loader.h \
	path.h \
	poi.h \
	router.h \
	viewer.h \
	util.h

libmappero_la_SOURCES = \
	debug.c \
	error.c \
	gpx.c \
	loader.c \
	path.c \
	router.c \
	viewer.c \
	util.c

libmappero_la_CFLAGS = \
	$(GCONF_CFLAGS) \
	$(GNOME_VFS_CFLAGS) \
	$(GTK_CFLAGS) \
	$(LIBXML2_CFLAGS) \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"mappero\" \
	-std=c99 -Wall -Wno-switch

libmappero_la_LIBADD = \
	$(GCONF_LIBS) \
	$(GNOME_VFS_LIBS) \
	$(GTK_LIBS) \
	$(LIBXML2_LIBS) \
	-lm -lrt

libmappero_la_LDFLAGS = \
	-version-info "@MAP_LT_CURRENT@":"@MAP_LT_REVISION@":"@MAP_LT_AGE@" \
	-Wl,--no-undefined \
	-export-dynamic \
	-export-symbols-regex "^map.*" \
	-rpath $(libdir)

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

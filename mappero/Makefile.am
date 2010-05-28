lib_LTLIBRARIES = libmappero.la

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

mappero_includedir = $(includedir)/mappero
mappero_include_HEADERS = \
	debug.h \
	globals.h \
	path.h \
	router.h \
	viewer.h

libmappero_la_SOURCES = \
	debug.c \
	path.c \
	router.c \
	viewer.c

libmappero_la_CFLAGS = \
	$(GCONF_CFLAGS) \
	$(GTK_CFLAGS)

libmappero_la_LIBADD = \
	$(GCONF_LIBS) \
	$(GTK_LIBS)

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

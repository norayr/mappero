pluginlib_LTLIBRARIES = libreittiopas.la

AM_CFLAGS = \
	-I$(top_srcdir) \
	$(GCONF_CFLAGS) \
	$(GTK_CFLAGS) \
	-module -avoid-version \
	-std=c99 -Wall -Wno-switch

LDADD = \
	-export-dynamic \
	-export-symbols-regex "^map_plugin_.*"

LIBADD = \
	$(top_builddir)/mappero/libmappero.la \
	$(GCONF_LIBS) \
	$(GTK_LIBS)

libreittiopas_la_SOURCES = \
	reittiopas.c
libreittiopas_la_CFLAGS = \
	$(HILDON_CFLAGS) \
	$(LIBXML2_CFLAGS)
libreittiopas_la_LIBADD = \
	$(HILDON_LIBS) \
	$(LIBXML2_LIBS)

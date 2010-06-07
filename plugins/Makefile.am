pluginlib_LTLIBRARIES = \
	libgoogle.la \
	libreittiopas.la

AM_CFLAGS = \
	-I$(top_srcdir) \
	$(GCONF_CFLAGS) \
	$(GTK_CFLAGS)

LDFLAGS = \
	$(top_builddir)/mappero/libmappero.la \
	$(top_builddir)/mappero-extras/libmappero-extras.la \
	$(GCONF_LIBS) \
	$(GTK_LIBS) \
	-export-dynamic \
	-export-symbols-regex "^map_plugin_.*" \
	-module -avoid-version \
	-Wl,--no-undefined

libreittiopas_la_SOURCES = \
	reittiopas.c
libreittiopas_la_CFLAGS = \
	$(HILDON_CFLAGS) \
	$(LIBXML2_CFLAGS) \
	$(AM_CFLAGS)
libreittiopas_la_LIBADD = \
	$(HILDON_LIBS) \
	$(LIBXML2_LIBS) \
	-lm

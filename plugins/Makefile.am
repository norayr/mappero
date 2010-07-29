pluginlib_LTLIBRARIES = \
	libgnuite.la \
	libgoogle.la \
	libreittiopas.la \
	libyandex.la

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

libgnuite_la_SOURCES = \
	gnuite.c
libgnuite_la_CFLAGS = \
	$(HILDON_CFLAGS) \
	$(AM_CFLAGS)
libgnuite_la_LIBADD = \
	$(HILDON_LIBS)

libgoogle_la_SOURCES = \
	google.c
libgoogle_la_CFLAGS = \
	$(HILDON_CFLAGS) \
	$(AM_CFLAGS)
libgoogle_la_LIBADD = \
	$(HILDON_LIBS)

libyandex_la_SOURCES = \
	yandex.c
libyandex_la_CFLAGS = \
	$(HILDON_CFLAGS) \
	$(AM_CFLAGS)
libyandex_la_LIBADD = \
	$(HILDON_LIBS)


noinst_LTLIBRARIES = libmappero-extras.la

AM_CFLAGS = \
	-I$(top_srcdir) \
	-std=c99 -Wall -Wno-switch

libmappero_extras_la_SOURCES = \
	dialog.c \
	error.c

libmappero_extras_la_CFLAGS = \
	$(GCONF_CFLAGS) \
	$(GTK_CFLAGS) \
	$(HILDON_CFLAGS)

libmappero_extras_la_LIBADD = \
	$(GCONF_LIBS) \
	$(GTK_LIBS) \
	$(HILDON_LIBS)


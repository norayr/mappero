#
# Copyright (C) 2006, 2007 John Costigan.
#
# This file is part of Maemo Mapper.
#
# Maemo Mapper is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Maemo Mapper is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Maemo Mapper.  If not, see <http://www.gnu.org/licenses/>.
#

bin_PROGRAMS = mappero

mappero_CPPFLAGS = \
	-I.. \
	$(QT_CFLAGS) \
	-I$(top_srcdir)/src/kineticscroller/include \
	-DMAP_PLUGIN_DIR=\"@libdir@/mappero.@MAP_ABI_VERSION@\" \
	-Wall -Wno-switch

mappero_LDADD = \
	$(QT_LIBS) \
	$(top_builddir)/src/kineticscroller/lib/libQtScroller.a

mappero_SOURCES = \
	configuration.cpp \
	controller.cpp \
	layer.cpp \
	main.cpp \
	map.cpp \
	projection.cpp \
	tile-cache.cpp \
	tile-download.cpp \
	tile.cpp \
	tiled-layer.cpp \
	types.cpp \
	view.cpp

nodist_mappero_SOURCES = \
	configuration.h.moc \
	controller.h.moc \
	layer.cpp.moc \
	map.cpp.moc \
	map.h.moc \
	tile-download.cpp.moc \
	tile-download.h.moc \
	tiled-layer.cpp.moc \
	view.h.moc

BUILT_SOURCES = \
	$(nodist_mappero_SOURCES)

CLEANFILES = \
	$(BUILT_SOURCES)

%.moc: %
	moc $(AM_CPPFLAGS) -i $< -o $@

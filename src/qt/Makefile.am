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
	-DMAP_PLUGIN_DIR=\"@libdir@/mappero.@MAP_ABI_VERSION@\" \
	-Wall -Wno-switch

mappero_LDADD = \
	$(QT_LIBS)

mappero_SOURCES = \
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
	controller.moc.hpp \
	map.moc.hpp \
	tile-download.moc.hpp \
	view.moc.hpp

BUILT_SOURCES = \
	$(nodist_mappero_SOURCES)

CLEANFILES = \
	$(BUILT_SOURCES)

%.moc.hpp: %.h
	moc $(AM_CPPFLAGS) -i $< -o $@

%.o:: %.moc.cpp
%.moc.cpp: %.cpp
	moc $(AM_CPPFLAGS) -i $< -o $@

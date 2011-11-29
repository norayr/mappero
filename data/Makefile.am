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
 
desktopdir = $(datadir)/applications/hildon
desktop_DATA = maemo-mapper.desktop

mimedir = $(datadir)/share/mime/packages
mime_DATA = mappero-mime.xml

icon_reittiopas = \
	icons/reittiopas/maemo-mapper-bus.png \
	icons/reittiopas/maemo-mapper-ferry.png \
	icons/reittiopas/maemo-mapper-metro.png \
	icons/reittiopas/maemo-mapper-train.png \
	icons/reittiopas/maemo-mapper-tram.png \
	icons/reittiopas/maemo-mapper-walk.png

icon_scalabledir = $(datadir)/icons/hicolor/scalable/hildon
icon_scalable_DATA = icons/scalable/maemo-mapper.png icons/scalable/maemo-mapper-wait.png \
	icons/scalable/maemo-mapper-point.png \
	icons/scalable/maemo-mapper-fullscreen.png \
	icons/scalable/maemo-mapper-go-to.png \
	icons/scalable/maemo-mapper-gps-enable.png \
	icons/scalable/maemo-mapper-gps-disable.png \
	icons/scalable/maemo-mapper-nav-bg.png \
	icons/scalable/maemo-mapper-nav-first-exit.png \
	icons/scalable/maemo-mapper-nav-second-exit.png \
	icons/scalable/maemo-mapper-nav-slight-left.png \
	icons/scalable/maemo-mapper-nav-slight-right.png \
	icons/scalable/maemo-mapper-nav-straight.png \
	icons/scalable/maemo-mapper-nav-third-exit.png \
	icons/scalable/maemo-mapper-nav-turn-left.png \
	icons/scalable/maemo-mapper-nav-turn-right.png \
	icons/scalable/maemo-mapper-nav-unknown.png \
	icons/scalable/maemo-mapper-path.png \
	icons/scalable/maemo-mapper-route.png \
	icons/scalable/maemo-mapper-settings.png \
	icons/scalable/maemo-mapper-view.png \
	icons/scalable/maemo-mapper-zoom-in.png \
	icons/scalable/maemo-mapper-zoom-out.png \
	$(icon_reittiopas)
icon_svg = \
	icons/scalable/svg/maemo-mapper-point.svg \
	icons/scalable/svg/maemo-mapper-fullscreen.svg \
	icons/scalable/svg/maemo-mapper-path.svg \
	icons/scalable/svg/maemo-mapper-settings.svg \
	icons/scalable/svg/maemo-mapper-route.svg \
	icons/scalable/svg/maemo-mapper-zoom-in.svg \
	icons/scalable/svg/maemo-mapper-zoom-out.svg
EXTRA_DIST = $(serviceentry_DATA) $(desktopentry_DATA) $(icon_scalable_DATA) \
	$(icon_svg)

SUBDIRS = sounds

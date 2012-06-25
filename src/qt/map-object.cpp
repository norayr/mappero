/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2012 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This file is part of Mappero.
 *
 * Mappero is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mappero is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mappero.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "debug.h"
#include "map.h"
#include "map-object.h"

using namespace Mappero;

namespace Mappero {

MapEvent::MapEvent(Map *map, bool dirty):
    m_map(map),
    m_centerChanged(dirty),
    m_zoomLevelChanged(dirty),
    m_sizeChanged(dirty)
{
}

void MapEvent::clear()
{
    m_centerChanged = false;
    m_zoomLevelChanged = false;
    m_sizeChanged = false;
}

}; // namespace

void MapObject::setMap(Map *map)
{
    if (map == m_map) return;

    if (m_map != 0) {
        m_map->removeObject(this);
    }

    m_map = map;

    if (map != 0) {
        map->addObject(this);

        // Inform the object that a map has been set
        MapEvent event(map, true);
        mapEvent(&event);
    }
}


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

#ifndef MAP_MAP_OBJECT_H
#define MAP_MAP_OBJECT_H

#include <QGraphicsItem>

namespace Mappero {

class Map;

class MapEvent {
public:
    ~MapEvent() {}

    Map *map() const { return m_map; }
    bool centerChanged() const { return m_centerChanged; }
    bool zoomLevelChanged() const { return m_zoomLevelChanged; }
    bool sizeChanged() const { return m_sizeChanged; }

private:
    friend class MapObject;
    friend class Map;
    friend class MapPrivate;
    MapEvent(Map *map, bool dirty = false);
    void clear();
    Map *m_map;
    bool m_centerChanged;
    bool m_zoomLevelChanged;
    bool m_sizeChanged;
};

class MapObject: public QGraphicsItem {
public:
    MapObject(): QGraphicsItem(), m_map(0) {}
    ~MapObject() {};

    void setMap(Map *map);
    Map *map() const { return m_map; }

    virtual void mapEvent(MapEvent *e) = 0;

private:
    Map *m_map;
};

};

#endif /* MAP_MAP_OBJECT_H */

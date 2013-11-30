/*
 * Copyright (C) 2011 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAP_TYPES_H
#define MAP_TYPES_H

#include <Mappero/types.h>
#include <QMetaType>
#include <QPoint>
#include <limits.h>
#include <math.h>

namespace Mappero {

class TiledLayer;

struct TileSpec
{
    TileSpec(int x, int y, int zoom, TiledLayer *layer):
        x(x), y(y), zoom(zoom), layer(layer) {}
    int x;
    int y;
    int zoom;
    TiledLayer *layer;
};

} // namespace

inline uint qHash(const Mappero::TileSpec &tile)
{
    return (tile.x & 0xff) |
        ((tile.y & 0xff) << 8) |
        ((tile.zoom & 0xff) << 16);
}

#include <QDebug>

QDebug operator<<(QDebug dbg, const Mappero::TileSpec &t);

inline bool operator==(const Mappero::TileSpec &t1, const Mappero::TileSpec &t2)
{
    return t1.x == t2.x && t1.y == t2.y && t1.zoom == t2.zoom &&
        t1.layer == t2.layer;
}

#endif /* MAP_TYPES_H */

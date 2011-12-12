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

#include "types.h"

using namespace Mappero;

QDebug operator<<(QDebug dbg, const GeoPoint &p)
{
    dbg.nospace() << "(" << p.lat << ", " << p.lon << ")";
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const Point &p)
{
    dbg.nospace() << "(" << p.x << ", " << p.y << ")";
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const TileSpec &t)
{
    dbg.nospace() << "(" << t.x << ", " << t.y << ")";
    return dbg.space();
}


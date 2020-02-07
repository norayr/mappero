/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2011-2020 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAP_TILE_CACHE_H
#define MAP_TILE_CACHE_H

#include "types.h"

#include <QMetaType>

namespace Mappero {

class Tile;

class TileCachePrivate;
class TileCache
{
public:
    TileCache();
    ~TileCache();

    Tile *tile(const TileSpec &spec, bool *found);
    Tile *find(const TileSpec &tileSpec) const;

private:
    TileCachePrivate *d_ptr;
    Q_DECLARE_PRIVATE(TileCache)
};

}; // namespace

#endif /* MAP_TILE_CACHE_H */

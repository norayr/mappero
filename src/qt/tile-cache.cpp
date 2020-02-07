/* vi: set et sw=4 ts=4 cino=t0,(0: */
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

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "debug.h"
#include "tile-cache.h"
#include "tile.h"
#include "tiled-layer.h"

#include <QPixmap>
#include <QQueue>

using namespace Mappero;

namespace Mappero {

struct TileData
{
    TileData(const TileSpec &spec, Tile *tile = 0):
        spec(spec), tile(tile) {}

    TileSpec spec;
    Tile *tile;
};

typedef QQueue<TileData> TileQueue;

class TileCachePrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(TileCache)

    TileCachePrivate(TileCache *tileCache, int maxSize):
        QObject(),
        q_ptr(tileCache),
        maxSize(maxSize)
    {
    }
    ~TileCachePrivate() {};

    Tile *tile(const TileSpec &tileSpec, bool *found);

private Q_SLOTS:
    void onTileDestroyed();

private:
    mutable TileCache *q_ptr;
    int maxSize;
    TileQueue tiles;
};

inline bool operator==(const TileData &t1, const TileData &t2)
{
    return t1.spec == t2.spec;
}

} // namespace

Tile *TileCachePrivate::tile(const TileSpec &tileSpec, bool *found)
{
    TileData tileData(tileSpec);

    int idx = tiles.indexOf(tileData);
    if (idx >= 0) {
        /* return this tile, after repositioning it at the end of the queue */
        tileData = tiles.takeAt(idx);
        *found = true;
    } else {
        /* We didn't find the tile; see if we are allowed to allocate a new
         * one, otherwise just return the first one of the queue, so it will be
         * reused */
        TiledLayer *layer =
            qobject_cast<TiledLayer*>(Layer::find(tileData.spec.layerId));
        if (Q_UNLIKELY(!layer)) {
            *found = false;
            return 0;
        }

        if (tiles.count() < maxSize) {
            tileData.tile = new Tile(layer);
            QObject::connect(tileData.tile, SIGNAL(destroyed()),
                             this, SLOT(onTileDestroyed()));
        } else {
            tileData.tile = tiles.head().tile;
            tileData.tile->setImage(QImage());
            tileData.tile->setParentItem(layer);
            tiles.dequeue();
        }
        *found = false;
    }
    tiles.enqueue(tileData);
    return tileData.tile;
}

void TileCachePrivate::onTileDestroyed()
{
    Tile *tile = reinterpret_cast<Tile*>(sender());
    Q_FOREACH(const TileData &tileData, tiles) {
        if (tileData.tile == tile) {
            tiles.removeOne(tileData);
            break;
        }
    }
}

TileCache::TileCache():
    d_ptr(new TileCachePrivate(this, 100))
{
}

TileCache::~TileCache()
{
    delete d_ptr;
}

Tile *TileCache::tile(const TileSpec &tileSpec, bool *found)
{
    Q_D(TileCache);

    return d->tile(tileSpec, found);
}

Tile *TileCache::find(const TileSpec &tileSpec) const
{
    Q_D(const TileCache);

    TileData tileData(tileSpec);
    int idx = d->tiles.indexOf(tileData);
    return idx >= 0 ? d->tiles[idx].tile : 0;
}

#include "tile-cache.moc"

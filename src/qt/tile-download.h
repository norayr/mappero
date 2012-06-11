/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
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

#ifndef MAP_TILE_DOWNLOAD_H
#define MAP_TILE_DOWNLOAD_H

#include "types.h"

#include <QMetaType>
#include <QObject>

namespace Mappero {

struct TileContents {
    TileContents(): image(QByteArray()), needsNetwork(false) {}

    QByteArray image;
    /* true if the tile image data could be improved by fetching it from the
     * network */
    bool needsNetwork;
};

class TileDownloadPrivate;
class TileDownload: public QObject
{
    Q_OBJECT

public:
    TileDownload(QObject *parent = 0);
    ~TileDownload();

    void requestTile(const TileSpec &spec, int priority);

Q_SIGNALS:
    void tileDownloaded(const TileSpec &tileSpec, TileContents tileContents);
    void onlineStateChanged(bool isOnline);

private:
    TileDownloadPrivate *d_ptr;
    Q_DECLARE_PRIVATE(TileDownload)
};

}; // namespace

#endif /* MAP_TILE_DOWNLOAD_H */

/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2009-2020 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAP_TILED_LAYER_H
#define MAP_TILED_LAYER_H

#include "layer.h"

#include <Mappero/Projection>
#include <QString>

namespace Mappero {

class TiledLayerPrivate;
class TiledLayer: public Layer
{
    Q_OBJECT
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY layerChanged);
    Q_PROPERTY(QString format READ format WRITE setFormat NOTIFY layerChanged);
    Q_PROPERTY(QString type READ typeName WRITE setTypeName \
               NOTIFY layerChanged);

public:
    struct Type {
        typedef QString (*MakeUrl)(const TiledLayer *layer,
                                   int zoom, int tileX, int tileY);
        const char *name;
        MakeUrl makeUrl;
        Projection::Type projectionType;
        static const Type *get(const char *name);
    };

    TiledLayer();
    TiledLayer(const QString &name, const QString &id,
               const QString &url, const QString &format, const Type *type,
               int minZoom, int maxZoom);
    virtual ~TiledLayer();

    void setUrl(const QString &url);
    QString url() const;

    void setFormat(const QString &format);
    QString format() const;

    void setTypeName(const QString &typeName);
    QString typeName() const;

    QString urlForTile(int zoom, int x, int y) const;
    QString tileFileName(int zoom, int x, int y) const;

    static const Projection *projectionFromLayerType(const Type *type);

    // reimplemented virtual functions:
    void mapEvent(MapEvent *event);

Q_SIGNALS:
    void layerChanged();

protected:
    bool tileInDB(int zoom, int x, int y) const;
    QPixmap tilePixmap(int zoom, int x, int y) const;

private:
    TiledLayerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(TiledLayer)
};

} // namespace

#endif /* MAP_TILED_LAYER_H */

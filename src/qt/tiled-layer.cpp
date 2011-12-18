/* vi: set et sw=4 ts=8 cino=t0,(0: */
/*
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include "types.h"
#include "controller.h"
#include "debug.h"
#include "map.h"
#include "tile-cache.h"
#include "tile-download.h"
#include "tile.h"
#include "tiled-layer.h"

#include <QFile>
#include <QHash>
#include <QObject>
#include <QPainter> // FIXME temp
#include <QStringBuilder>

using namespace Mappero;

static QString xyz_get_url(const TiledLayer *layer,
                           int zoom, int tileX, int tileY);
static QString xyz_inv_get_url(const TiledLayer *layer,
                               int zoom, int tileX, int tileY);
static QString xyz_signed_get_url(const TiledLayer *layer,
                                  int zoom, int tileX, int tileY);
static QString yandex_get_url(const TiledLayer *layer,
                              int zoom, int tileX, int tileY);

static const TiledLayer::Type layerTypes[] = {
    { "XYZ", xyz_get_url, Projection::GOOGLE },
    { "XYZ_SIGNED", xyz_signed_get_url, Projection::GOOGLE },
    { "XYZ_INV", xyz_inv_get_url, Projection::GOOGLE },
    /* not implemented
    { "QUAD_QRST", quad_qrst_get_url, Projection::GOOGLE },
    { "QUAD_ZERO", quad_zero_get_url, Projection::GOOGLE },
    */
    { "YANDEX", yandex_get_url, Projection::YANDEX },
    { name: NULL, }
};

const TiledLayer::Type *TiledLayer::Type::get(const char *name)
{
    for (int i = 0; layerTypes[i].name != 0; i++)
        if (strcmp(name, layerTypes[i].name) == 0) return &layerTypes[i];
    return 0;
}

namespace Mappero {

typedef QHash<TileSpec, Tile *> TileQueue;

class TiledLayerPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(TiledLayer)

    TiledLayerPrivate(TiledLayer *tiledLayer,
                      const QString &name, const QString &id,
                      const QString &url, const QString &format,
                      const TiledLayer::Type *type):
        q_ptr(tiledLayer),
        name(name),
        id(id),
        url(url),
        format(format),
        type(type),
        center(0, 0),
        zoomLevel(-1)
    {
        /* FIXME: get it from the configuration */
        baseDir = QString::fromLatin1("/home/user/MyDocs/.maps/");

        tileDownload = Controller::instance()->tileDownload();
        tileCache = Controller::instance()->tileCache();
        QObject::connect(tileDownload,
                         SIGNAL(tileDownloaded(const TileSpec &, QByteArray)),
                         this,
                         SLOT(onTileDownloaded(const TileSpec &, QByteArray)));
    }

    int zoomLevelFromMap(const Map *map) const
    {
        return (int)map->zoomLevel();
    }

    Unit pixel2unit(int pixel) const { return pixel << zoomLevel; }
    void loadTiles(const QPoint &start, const QPoint stop);

private Q_SLOTS:
    void onTileDownloaded(const TileSpec &tileSpec, QByteArray tileData);

private:
    mutable TiledLayer *q_ptr;
    QString name;
    QString id;
    QString url;
    QString format;
    const TiledLayer::Type *type;
    QString baseDir;
    TileDownload *tileDownload;
    TileCache *tileCache;
    TileQueue tileQueue;

    Point center;
    int zoomLevel;
    QSize viewportHalfSize;
};
}; // namespace

static QString xyz_get_url(const TiledLayer *layer,
                           int zoom, int tileX, int tileY)
{
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), layer->url().toUtf8().constData(),
             tileX, tileY, zoom - (MAX_ZOOM - 16));
    return QString::fromUtf8(buffer);
}

static QString xyz_inv_get_url(const TiledLayer *layer,
                               int zoom, int tileX, int tileY)
{
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), layer->url().toUtf8().constData(),
             MAX_ZOOM + 1 - zoom, tileX, tileY);
    return QString::fromUtf8(buffer);
}

static QString xyz_signed_get_url(const TiledLayer *layer,
                                  int zoom, int tileX, int tileY)
{
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), layer->url().toUtf8().constData(),
             tileX,
             (1 << (MAX_ZOOM - zoom)) - tileY - 1,
             zoom - (MAX_ZOOM - 17));
    return QString::fromUtf8(buffer);
}

static QString yandex_get_url(const TiledLayer *layer,
                              int zoom, int tileX, int tileY)
{
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), layer->url().toUtf8().constData(),
             tileX, tileY, MAX_ZOOM + 1 - zoom);
    return QString::fromUtf8(buffer);
}

const Projection *TiledLayer::projectionFromLayerType(const Type *type)
{
    return Projection::get(type->projectionType);
}

void TiledLayerPrivate::loadTiles(const QPoint &start, const QPoint stop)
{
    Q_Q(TiledLayer);

    QPoint p = start * (TILE_SIZE_PIXELS << zoomLevel);
    int &x = p.rx();
    int &y = p.ry();

    foreach (QGraphicsItem *item, q->childItems()) {
        item->setVisible(false);
    }

    x -= center.x;
    y -= center.y;
    p /= (1 << zoomLevel);
    int y0 = y;
    for (int tx = start.x(); tx <= stop.x(); tx++, x += TILE_SIZE_PIXELS) {
        y = y0;
        for (int ty = start.y(); ty <= stop.y(); ty++, y+= TILE_SIZE_PIXELS) {
            TileSpec tileSpec(tx, ty, zoomLevel, q);
            bool found;

            Tile *tile = tileCache->tile(tileSpec, &found);
            if (!found) {
                tileQueue.insert(tileSpec, tile);
                tileDownload->requestTile(tileSpec, 0);
            } else {
                tile->setVisible(true);
            }
            tile->setPos(x, y);
        }
    }
}

void TiledLayerPrivate::onTileDownloaded(const TileSpec &tileSpec,
                                         QByteArray tileData)
{
    DEBUG() << "Downloaded tile: " << tileSpec;
    if (tileData.isEmpty()) {
        DEBUG() << "No tile data!";
        return;
    }

    TileQueue::iterator i = tileQueue.find(tileSpec);
    if (i != tileQueue.end()) {
        Tile *tile = i.value();
        QPixmap pixmap;
        pixmap.loadFromData(tileData);
        tile->setPixmap(pixmap);
        tile->setVisible(true);
    }
}

TiledLayer::TiledLayer(const QString &name, const QString &id,
                       const QString &url, const QString &format,
                       const Type *type):
    Layer(id, projectionFromLayerType(type)),
    d_ptr(new TiledLayerPrivate(this, name, id, url, format, type))
{
}

TiledLayer::~TiledLayer()
{
    delete d_ptr;
}

const QString TiledLayer::url() const
{
    Q_D(const TiledLayer);
    return d->url;
}

QString TiledLayer::urlForTile(int zoom, int x, int y) const
{
    Q_D(const TiledLayer);
    return d->type->makeUrl(this, zoom, x, y);
}

QString TiledLayer::tileFileName(int zoom, int x, int y) const
{
    Q_D(const TiledLayer);
    return d->baseDir % QString("%1/%2/%3/%4.%5")
        .arg(d->id)
        .arg(21 - zoom).arg(x).arg(y)
        .arg(d->format);
}

bool TiledLayer::tileInDB(int zoom, int x, int y) const
{
    qDebug("%d, %d, %d", zoom, x, y);

    QString path = tileFileName(zoom, x, y);
    return QFile::exists(path);
}

QPixmap TiledLayer::tilePixmap(int zoom, int x, int y) const
{
    qDebug("%d, %d, %d", zoom, x, y);

    QString path = tileFileName(zoom, x, y);
    return QPixmap(path);
}

void TiledLayer::paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *widget)
{
    painter->drawRoundedRect(-10, -10, 20, 20, 5, 5);
}

void TiledLayer::mapChanged()
{
    Q_D(TiledLayer);

    DEBUG() << Q_FUNC_INFO;
    Map *map = this->map();

    Point center = map->centerUnits();
    int zoomLevel = d->zoomLevelFromMap(map);
    if (zoomLevel < 0) return;
    QSize viewportHalfSize = map->boundingRect().size().toSize() / 2;

    if (center == d->center &&
        zoomLevel == d->zoomLevel &&
        viewportHalfSize == d->viewportHalfSize) {
        // nothing has changed for us
        DEBUG() << "Ignoring map changes";
        return;
    }

    d->center = center;
    d->zoomLevel = zoomLevel;
    d->viewportHalfSize = viewportHalfSize;

    DEBUG() << "center:" << map->center();
    DEBUG() << "center (units):" << map->centerUnits();

    int halfLength = qMax(viewportHalfSize.width(), viewportHalfSize.height());
    Unit halfLengthUnit = d->pixel2unit(halfLength);

    QPoint tileStart =
        Point(center.x - halfLengthUnit,
              center.y - halfLengthUnit).
        toTile(zoomLevel);
    QPoint tileStop =
        Point(center.x + halfLengthUnit + TILE_SIZE_PIXELS - 1,
              center.y + halfLengthUnit + TILE_SIZE_PIXELS - 1).
        toTile(zoomLevel);

    d->loadTiles(tileStart, tileStop);
}

#include "tiled-layer.moc.cpp"

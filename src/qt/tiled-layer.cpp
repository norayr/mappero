/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2010-2012 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include "configuration.h"
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
static QString quad_qrst_get_url(const TiledLayer *layer,
                                 int zoom, int tileX, int tileY);
static QString quad_zero_get_url(const TiledLayer *layer,
                                 int zoom, int tileX, int tileY);

static const TiledLayer::Type layerTypes[] = {
    { "XYZ", xyz_get_url, Projection::GOOGLE },
    { "XYZ_SIGNED", xyz_signed_get_url, Projection::GOOGLE },
    { "XYZ_INV", xyz_inv_get_url, Projection::GOOGLE },
    { "QUAD_QRST", quad_qrst_get_url, Projection::GOOGLE },
    { "QUAD_ZERO", quad_zero_get_url, Projection::GOOGLE },
    { "YANDEX", yandex_get_url, Projection::YANDEX },
    { NULL, NULL, Projection::LAST }
};

const TiledLayer::Type *TiledLayer::Type::get(const char *name)
{
    for (int i = 0; layerTypes[i].name != 0; i++)
        if (strcmp(name, layerTypes[i].name) == 0) return &layerTypes[i];
    return 0;
}

namespace Mappero {

class TiledLayerPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(TiledLayer)

    TiledLayerPrivate(TiledLayer *tiledLayer):
        q_ptr(tiledLayer),
        url(),
        format(),
        type(0),
        fetchMissingTiles(false),
        center(0, 0),
        zoomLevel(-1)
    {
        Controller *controller = Controller::instance();

        baseDir = controller->configuration()->mapCacheDir();
        tileDownload = controller->tileDownload();
        tileCache = controller->tileCache();
        QObject::connect(tileDownload,
                         SIGNAL(tileDownloaded(const TileSpec &,TileContents)),
                         this,
                         SLOT(onTileDownloaded(const TileSpec &,TileContents)));

        QObject::connect(tileDownload,
                         SIGNAL(onlineStateChanged(bool)),
                         this,
                         SLOT(onOnlineStateChanged(bool)));
    }

    int zoomLevelFromMap(const Map *map) const
    {
        return (int)map->zoomLevel();
    }

    Unit pixel2unit(int pixel) const { return pixel << zoomLevel; }
    void loadTiles(const QPoint &start, const QPoint stop);

private Q_SLOTS:
    void onTileDownloaded(const TileSpec &tileSpec, TileContents tileContents);
    void onOnlineStateChanged(bool isOnline);

private:
    mutable TiledLayer *q_ptr;
    QString url;
    QString format;
    const TiledLayer::Type *type;
    QString baseDir;
    TileDownload *tileDownload;
    TileCache *tileCache;
    bool fetchMissingTiles;

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

static void
map_convert_coords_to_quadtree_string(int x, int y, int zoomLevel,
                                      char *buffer, char initial,
                                      const char *const quadrant)
{
    char *ptr = buffer;
    int n;

    if (initial)
        *ptr++ = initial;

    for (n = MAX_ZOOM - zoomLevel; n >= 0; n--)
    {
        int xbit = (x >> n) & 1;
        int ybit = (y >> n) & 1;
        *ptr++ = quadrant[xbit + 2 * ybit];
    }
    *ptr++ = '\0';
}

static QString quad_qrst_get_url(const TiledLayer *layer,
                                 int zoom, int tileX, int tileY)
{
    char location[MAX_ZOOM + 2];
    map_convert_coords_to_quadtree_string(tileX, tileY, zoom,
                                          location, 't', "qrts");
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), layer->url().toUtf8().constData(),
             location);
    return QString::fromUtf8(buffer);
}

static QString quad_zero_get_url(const TiledLayer *layer,
                                 int zoom, int tileX, int tileY)
{
    char location[MAX_ZOOM + 2];
    map_convert_coords_to_quadtree_string(tileX, tileY, zoom,
                                          location, '\0', "0123");
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), layer->url().toUtf8().constData(),
             location);
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

    foreach (QQuickItem *item, q->childItems()) {
        item->setVisible(false);
    }

    x -= center.x();
    y -= center.y();
    p /= (1 << zoomLevel);
    int worldSizeTiles = 1 << (MAX_ZOOM + 1 - zoomLevel);
    int y0 = y;
    for (int tx = start.x(); tx <= stop.x(); tx++, x += TILE_SIZE_PIXELS) {
        y = y0;
        int mx = tx % worldSizeTiles;
        if (mx < 0) mx += worldSizeTiles;
        for (int ty = start.y(); ty <= stop.y(); ty++, y+= TILE_SIZE_PIXELS) {
            int my = ty % worldSizeTiles;
            if (my < 0) my += worldSizeTiles;
            TileSpec tileSpec(mx, my, zoomLevel, q->id());
            bool found;

            Tile *tile = tileCache->tile(tileSpec, &found);
            if (!found || (fetchMissingTiles && tile->needsNetwork())) {
                tileDownload->requestTile(tileSpec, 0);
            } else {
                tile->setVisible(true);
            }
            tile->setX(x);
            tile->setY(y);
        }
    }

    fetchMissingTiles = false;
}

void TiledLayerPrivate::onTileDownloaded(const TileSpec &tileSpec,
                                         TileContents tileContents)
{
    if (tileContents.image.isEmpty()) {
        DEBUG() << "No tile data!";
        return;
    }

    Tile *tile = tileCache->find(tileSpec);
    if (tile != 0) {
        tile->setTileContents(tileContents);
        /* Don't re-show tiles which don't belong here */
        if (tileSpec.zoom == zoomLevel)
            tile->setVisible(true);
    }
}

void TiledLayerPrivate::onOnlineStateChanged(bool isOnline)
{
    if (!isOnline) return;

    DEBUG() << "Network is available";

    Q_Q(TiledLayer);
    /* refresh the view, to fetch any missing tiles */
    fetchMissingTiles = true;
    q->mapEvent(0);
}

TiledLayer::TiledLayer():
    Layer(),
    d_ptr(new TiledLayerPrivate(this))
{
}

TiledLayer::~TiledLayer()
{
    delete d_ptr;
}

void TiledLayer::setUrl(const QString &url)
{
    Q_D(TiledLayer);
    d->url = url;
    queueLayerChanged();
}

QString TiledLayer::url() const
{
    Q_D(const TiledLayer);
    return d->url;
}

void TiledLayer::setFormat(const QString &format)
{
    Q_D(TiledLayer);
    d->format = format;
    queueLayerChanged();
}

QString TiledLayer::format() const
{
    Q_D(const TiledLayer);
    return d->format;
}

void TiledLayer::setTypeName(const QString &typeName)
{
    Q_D(TiledLayer);
    QByteArray ba = typeName.toLatin1();
    d->type = Type::get(ba.constData());
    setProjection(projectionFromLayerType(d->type));
    queueLayerChanged();
}

QString TiledLayer::typeName() const
{
    Q_D(const TiledLayer);
    return d->type->name;
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
        .arg(id())
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

void TiledLayer::mapEvent(MapEvent *event)
{
    Q_UNUSED(event);
    Q_D(TiledLayer);

    Map *map = this->map();

    Point center = map->centerUnits();
    int zoomLevel = d->zoomLevelFromMap(map);
    if (zoomLevel < 0) return;
    QSize viewportHalfSize = map->boundingRect().size().toSize() / 2;

    d->center = center;
    d->zoomLevel = zoomLevel;
    d->viewportHalfSize = viewportHalfSize;

    int halfLength = qMax(viewportHalfSize.width(), viewportHalfSize.height());
    Unit halfLengthUnit = d->pixel2unit(halfLength);

    QPoint tileStart =
        Point(center.x() - halfLengthUnit,
              center.y() - halfLengthUnit).
        toTile(zoomLevel);
    QPoint tileStop =
        Point(center.x() + halfLengthUnit + TILE_SIZE_PIXELS - 1,
              center.y() + halfLengthUnit + TILE_SIZE_PIXELS - 1).
        toTile(zoomLevel);

    d->loadTiles(tileStart, tileStop);
}

#include "tiled-layer.moc"

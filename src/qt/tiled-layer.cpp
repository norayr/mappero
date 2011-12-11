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
#include "debug.h"
#include "map.h"
#include "tiled-layer.h"

#include <QFile>
#include <QGraphicsView>
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
class TiledLayerPrivate
{
    Q_DECLARE_PUBLIC(TiledLayer)

    TiledLayerPrivate(const QString &name, const QString &id,
                      const QString &url, const QString &format,
                      const TiledLayer::Type *type):
        name(name),
        id(id),
        url(url),
        format(format),
        type(type)
    {
        /* FIXME: get it from the configuration */
        baseDir = QString::fromLatin1("/home/user/MyDocs/.maps/");
    }

    QString name;
    QString id;
    QString url;
    QString format;
    const TiledLayer::Type *type;
    QString baseDir;
private:
    mutable TiledLayer *q_ptr;
};
};

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

TiledLayer::TiledLayer(const QString &name, const QString &id,
                       const QString &url, const QString &format,
                       const Type *type):
    Layer(id, projectionFromLayerType(type)),
    d_ptr(new TiledLayerPrivate(name, id, url, format, type))
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
    DEBUG() << Q_FUNC_INFO;
    painter->drawRoundedRect(-10, -10, 20, 20, 5, 5);

    QGraphicsScene *scene = this->scene();
    if (!scene) return;

    QGraphicsView *view = scene->views().first();
    if (!view) return;

    DEBUG() << "center:" << map()->center();
    DEBUG() << "center (units):" << map()->centerUnits();
}


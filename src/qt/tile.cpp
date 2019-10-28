/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2011-2019 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include "tile-download.h"
#include "tile.h"
#include "tiled-layer.h"

#include <QImage>
#include <QSGSimpleTextureNode>
#include <QQuickWindow>

using namespace Mappero;

Tile::Tile(TiledLayer *parent):
    QQuickItem(parent),
    m_needsNetwork(true)
{
    setFlags(QQuickItem::ItemHasContents);
}

Tile::~Tile()
{
}

void Tile::setImage(const QImage &image)
{
    m_image = image;
    setWidth(image.width());
    setHeight(image.height());
    update();
}

void Tile::setTileContents(const TileContents &tileContents)
{
    QImage image;
    image.loadFromData(tileContents.image);
    setImage(image);

    m_needsNetwork = tileContents.needsNetwork;
}

QSGNode *Tile::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    if (m_image.isNull()) return node;

    QSGSimpleTextureNode *n = static_cast<QSGSimpleTextureNode*>(node);
    if (!n) {
        n = new QSGSimpleTextureNode;
        n->setFiltering(QSGTexture::Linear);
    }
    n->setRect(boundingRect());
    delete n->texture();
    n->setTexture(window()->createTextureFromImage(m_image));
    m_image = QImage();
    return n;
}

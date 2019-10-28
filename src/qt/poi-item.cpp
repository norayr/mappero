/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2012-2019 Alberto Mardegan <mardy@users.sourceforge.net>
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

#include "debug.h"
#include "poi-item.h"

#include <QSGTransformNode>

using namespace Mappero;

#define DEFAULT_WIDTH 200
#define DEFAULT_HEIGHT 200

PoiItem::PoiItem():
    QQuickItem(0)
{
    /* TODO
    setFlags(QGraphicsItem::ItemIgnoresTransformations |
             QGraphicsItem::ItemSendsGeometryChanges);
             */

    setImplicitWidth(DEFAULT_WIDTH);
    setImplicitHeight(DEFAULT_HEIGHT);
}

PoiItem::~PoiItem()
{
}

QSGNode *PoiItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *data)
{
    QSGTransformNode *transformNode = data->transformNode;
    DEBUG() << transformNode->matrix();
    return node;
}

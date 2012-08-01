/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2012 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAP_PATH_ITEM_H
#define MAP_PATH_ITEM_H

#include "path.h"

#include <QColor>
#include <QObject>

namespace Mappero {

class PathItemPrivate;
class PathItem: public QObject
{
    Q_OBJECT
    Q_PROPERTY(Path path READ path WRITE setPath NOTIFY pathChanged);
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged);

public:
    PathItem(QObject *parent = 0);
    ~PathItem();

    void setPath(const Path &path);
    Path path() const;
    Path &path();

    void setColor(const QColor &color);
    QColor color() const;

public Q_SLOTS:
    void loadFile(const QString &fileName);
    void saveFile(const QString &fileName) const;

Q_SIGNALS:
    void colorChanged();
    void pathChanged();

private:
    PathItemPrivate *d_ptr;
    Q_DECLARE_PRIVATE(PathItem)
};

}; // namespace

#endif /* MAP_PATH_ITEM_H */

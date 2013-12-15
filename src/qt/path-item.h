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

#include <Mappero/Path>
#include <QColor>
#include <QDateTime>
#include <QQuickItem>

class QAbstractListModel;
class QQmlComponent;

namespace Mappero {

class PathItemPrivate;
class PathItem: public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(Path path READ path WRITE setPath NOTIFY pathChanged);
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged);
    Q_PROPERTY(bool empty READ isEmpty NOTIFY pathChanged);
    Q_PROPERTY(int timeOffset READ timeOffset WRITE setTimeOffset \
               NOTIFY timeOffsetChanged);
    Q_PROPERTY(QDateTime startTime READ startTime NOTIFY timesChanged);
    Q_PROPERTY(QDateTime endTime READ endTime NOTIFY timesChanged);
    Q_PROPERTY(int length READ length NOTIFY pathChanged);
    Q_PROPERTY(QQmlComponent *wayPointDelegate READ wayPointDelegate \
               WRITE setWayPointDelegate NOTIFY wayPointDelegateChanged);
    Q_PROPERTY(QAbstractListModel *wayPointModel READ wayPointModel CONSTANT);

public:
    PathItem(QQuickItem *parent = 0);
    ~PathItem();

    void setPath(const Path &path);
    Path path() const;
    Path &path();

    void setColor(const QColor &color);
    QColor color() const;

    bool isEmpty() const;

    void setTimeOffset(int offset);
    int timeOffset() const;

    QDateTime startTime() const;
    QDateTime endTime() const;

    qreal length() const;

    void setWayPointDelegate(QQmlComponent *delegate);
    QQmlComponent *wayPointDelegate() const;

    QAbstractListModel *wayPointModel() const;

    Q_INVOKABLE GeoPoint positionAt(const QDateTime &time) const;
    Q_INVOKABLE QRectF itemArea() const;

public Q_SLOTS:
    void clear();
    void loadFile(const QUrl &fileName);
    void saveFile(const QUrl &fileName) const;

Q_SIGNALS:
    void colorChanged();
    void pathChanged();
    void timeOffsetChanged();
    void timesChanged();
    void wayPointDelegateChanged();

private:
    PathItemPrivate *d_ptr;
    Q_DECLARE_PRIVATE(PathItem)
};

}; // namespace

#endif /* MAP_PATH_ITEM_H */

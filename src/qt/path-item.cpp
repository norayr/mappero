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
#include "path-item.h"

#include <QAbstractListModel>
#include <QDateTime>
#include <QUrl>

using namespace Mappero;

namespace Mappero {

class WayPointModel: public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        GeoPointRole = Qt::UserRole + 1,
        TextRole,
        TimeRole,
        DataRole,
    };

    WayPointModel(QObject *parent = 0);

    void setPath(const Path &path);

    Q_INVOKABLE QVariant get(int row, const QString &role) const;

    // reimplemented virtual methods
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

Q_SIGNALS:
    void countChanged();

private:
    Path m_path;
    QHash<int, QByteArray> m_roles;
};

} // namespace

WayPointModel::WayPointModel(QObject *parent):
    QAbstractListModel(parent)
{
    m_roles[GeoPointRole] = "geoPoint";
    m_roles[TextRole] = "text";
    m_roles[TimeRole] = "time";
    m_roles[DataRole] = "data";

    QObject::connect(this, SIGNAL(modelReset()),
                     this, SIGNAL(countChanged()));
}

void WayPointModel::setPath(const Path &path)
{
    beginResetModel();
    m_path = path;
    endResetModel();
}

QVariant WayPointModel::get(int row, const QString &roleName) const
{
    int role = roleNames().key(roleName.toLatin1(), -1);
    return data(index(row, 0), role);
}

int WayPointModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_path.wayPointCount();
}

QVariant WayPointModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row >= m_path.wayPointCount()) return QVariant();

    switch (role) {
    case GeoPointRole:
        return QVariant::fromValue(m_path.wayPointAt(row).geo);
    case TextRole:
        return m_path.wayPointText(row);
    case TimeRole:
        return QDateTime::fromTime_t(m_path.wayPointAt(row).time);
    case DataRole:
        return m_path.wayPointData(row);
    default:
        qWarning() << "Unknown role ID:" << role;
        return QVariant();
    }
}

QHash<int, QByteArray> WayPointModel::roleNames() const
{
    return m_roles;
}

namespace Mappero {

class PathItemPrivate
{
    Q_DECLARE_PUBLIC(PathItem)

public:
    inline PathItemPrivate(PathItem *tracker);
    PathItemPrivate() {}

private:
    mutable PathItem *q_ptr;
    Path path;
    QColor color;
    qreal opacity;
    int offset;
    QQmlComponent *wayPointDelegate;
    mutable WayPointModel *wayPointModel;
};

} // namespace

inline PathItemPrivate::PathItemPrivate(PathItem *tracker):
    q_ptr(tracker),
    opacity(1.0),
    offset(0),
    wayPointDelegate(0),
    wayPointModel(0)
{
}

PathItem::PathItem(QQuickItem *parent):
    QQuickItem(parent),
    d_ptr(new PathItemPrivate(this))
{
    QObject::connect(this, SIGNAL(pathChanged()),
                     this, SIGNAL(timesChanged()));
}

PathItem::~PathItem()
{
    delete d_ptr;
}

void PathItem::setPath(const Path &path)
{
    Q_D(PathItem);
    d->path = path;
    if (d->wayPointModel) d->wayPointModel->setPath(path);
    Q_EMIT pathChanged();
}

Path PathItem::path() const
{
    Q_D(const PathItem);
    return d->path;
}

Path &PathItem::path()
{
    Q_D(PathItem);
    return d->path;
}

void PathItem::setColor(const QColor &color)
{
    Q_D(PathItem);
    d->color = color;
    Q_EMIT colorChanged();
}

QColor PathItem::color() const
{
    Q_D(const PathItem);
    return d->color;
}

bool PathItem::isEmpty() const
{
    Q_D(const PathItem);
    return d->path.isEmpty();
}

void PathItem::setTimeOffset(int offset)
{
    Q_D(PathItem);

    if (offset == d->offset) return;

    d->offset = offset;
    Q_EMIT timeOffsetChanged();
    Q_EMIT timesChanged();
}

int PathItem::timeOffset() const
{
    Q_D(const PathItem);
    return d->offset;
}


QDateTime PathItem::startTime() const
{
    Q_D(const PathItem);
    if (d->path.isEmpty()) return QDateTime();

    return QDateTime::fromTime_t(d->path.firstPoint().time).addSecs(d->offset);
}

QDateTime PathItem::endTime() const
{
    Q_D(const PathItem);
    if (d->path.isEmpty()) return QDateTime();

    return QDateTime::fromTime_t(d->path.lastPoint().time).addSecs(d->offset);
}

qreal PathItem::length() const
{
    Q_D(const PathItem);
    return d->path.length();
}

void PathItem::setWayPointDelegate(QQmlComponent *delegate)
{
    Q_D(PathItem);

    if (delegate == d->wayPointDelegate) return;

    d->wayPointDelegate = delegate;
    Q_EMIT wayPointDelegateChanged();
}

QQmlComponent *PathItem::wayPointDelegate() const
{
    Q_D(const PathItem);
    return d->wayPointDelegate;
}

QAbstractListModel *PathItem::wayPointModel() const
{
    Q_D(const PathItem);
    if (!d->wayPointModel) {
        d->wayPointModel = new WayPointModel(const_cast<PathItem*>(this));
        d->wayPointModel->setPath(d->path);
    }
    return d->wayPointModel;
}

GeoPoint PathItem::positionAt(const QDateTime &time) const
{
    Q_D(const PathItem);

    time_t t = time.addSecs(-d->offset).toTime_t();
    PathPoint p = d->path.positionAt(t);
    return p.geo;
}

QRectF PathItem::itemArea() const
{
    Q_D(const PathItem);
    return d->path.boundingRect();
}

void PathItem::clear()
{
    Path path;
    setPath(path);
}

void PathItem::loadFile(const QUrl &fileName)
{
    Path path;
    path.load(fileName.toLocalFile());
    setPath(path);
}

void PathItem::saveFile(const QUrl &fileName) const
{
    path().save(fileName.toLocalFile());
}

#include "path-item.moc"

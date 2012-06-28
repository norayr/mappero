/* vi: set et sw=4 ts=8 cino=t0,(0: */
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

#include "debug.h"
#include "taggable-area.h"
#include "taggable.h"

#include <QAbstractListModel>
#include <QDateTime>
#include <QFileInfo>
#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>

using namespace Mappero;

namespace Mappero {

class TaggableModel: public QAbstractListModel
{
    Q_OBJECT

public:
    enum TaggableModelRoles {
        TaggableRole = Qt::UserRole + 1,
        FileNameRole,
        TimeRole,
        GeoPointRole,
    };

    TaggableModel(QObject *parent = 0);
    void addUrls(const QList<QUrl> &urlList);

    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

private:
    QList<Taggable *> taggables;
};

class TaggableAreaPrivate
{
    Q_DECLARE_PUBLIC(TaggableArea)

    TaggableAreaPrivate(TaggableArea *taggableArea);

private:
    TaggableModel *model;
    mutable TaggableArea *q_ptr;
};
}; // namespace

TaggableModel::TaggableModel(QObject *parent):
    QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles[TaggableRole] = "taggable";
    roles[FileNameRole] = "fileName";
    roles[TimeRole] = "time";
    roles[GeoPointRole] = "geoPoint";
    setRoleNames(roles);
}

void TaggableModel::addUrls(const QList<QUrl> &urlList)
{
    int first = rowCount();
    int last = first + urlList.count() - 1;
    beginInsertRows(QModelIndex(), first, last);
    foreach (const QUrl &url, urlList) {
        Taggable *taggable = new Taggable(this);
        taggable->setFileName(url.toLocalFile());
        taggables.append(taggable);
    }
    endInsertRows();
}

QVariant TaggableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();

    Taggable *taggable = taggables[index.row()];
    switch (role) {
    case TaggableRole:
        return QVariant::fromValue(taggable);
    case FileNameRole:
        {
            QFileInfo fi(taggable->fileName());
            return fi.fileName();
        }
    case TimeRole:
        return QDateTime::fromTime_t(taggable->time());
    case GeoPointRole:
        return taggable->hasLocation() ?
            QVariant::fromValue(taggable->location()) : QVariant();
    default:
        return QVariant();
    }
}

int TaggableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return taggables.count();
}

TaggableAreaPrivate::TaggableAreaPrivate(TaggableArea *taggableArea):
    model(new TaggableModel(taggableArea)),
    q_ptr(taggableArea)
{
    qmlRegisterType<Mappero::Taggable>();
    qmlRegisterType<Mappero::TaggableModel>();
}

TaggableArea::TaggableArea():
    QDeclarativeItem(0),
    d_ptr(new TaggableAreaPrivate(this))
{
    setAcceptDrops(true);
}

TaggableArea::~TaggableArea()
{
    delete d_ptr;
}

TaggableModel *TaggableArea::model() const
{
    Q_D(const TaggableArea);
    DEBUG() << "called";
    return d->model;
}

void TaggableArea::dropEvent(QGraphicsSceneDragDropEvent *e)
{
    Q_D(TaggableArea);
    const QMimeData *mimeData = e->mimeData();

    if (mimeData->hasUrls()) {
        d->model->addUrls(mimeData->urls());
    }
}

#include "taggable-area.moc"

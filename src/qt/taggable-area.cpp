/* vi: set et sw=4 ts=4 cino=t0,(0: */
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

#include "controller.h"
#include "debug.h"
#include "taggable-area.h"
#include "taggable-selection.h"
#include "taggable.h"

#include <QDateTime>
#include <QFileInfo>
#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>

using namespace Mappero;

namespace Mappero {

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
    QAbstractListModel(parent),
    _selection(new TaggableSelection(this)),
    checkChangesQueued(false),
    lastChangesTime(Controller::clock())
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
    bool wasEmpty = isEmpty();

    int first = rowCount();
    int last = first + urlList.count() - 1;
    beginInsertRows(QModelIndex(), first, last);
    foreach (const QUrl &url, urlList) {
        Taggable *taggable = new Taggable(this);
        QObject::connect(taggable, SIGNAL(locationChanged()),
                         this, SLOT(onTaggableChanged()));
        taggable->setFileName(url.toLocalFile());
        taggables.append(taggable);
    }
    endInsertRows();

    if (wasEmpty && !isEmpty()) {
        Q_EMIT isEmptyChanged();
    }
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

bool TaggableModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 1; i <= count; i++) {
        taggables.removeAt(row + count - i);
    }
    endRemoveRows();

    if (count > 0 && isEmpty()) {
        Q_EMIT isEmptyChanged();
    }
    return true;
}

void TaggableModel::onTaggableChanged()
{
    if (checkChangesQueued) return;

    QMetaObject::invokeMethod(this, "checkChanges", Qt::QueuedConnection);
    checkChangesQueued = true;
}

void TaggableModel::checkChanges()
{
    int count = taggables.count();
    int indexMin = -1;
    int indexMax = -1;
    for (int i = 0; i < count; i++) {
        Taggable *taggable = taggables.at(i);
        if (taggable->lastChange() > lastChangesTime) {
            if (indexMin == -1) indexMin = i;
            indexMax = i;
        }
    }

    if (indexMin != -1) {
        DEBUG() << "min" << indexMin << "max" << indexMax;
        Q_EMIT dataChanged(index(indexMin, 0), index(indexMax, 0));
    }

    checkChangesQueued = false;
    lastChangesTime = Controller::clock();
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

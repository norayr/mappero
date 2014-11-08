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
#include <QDropEvent>
#include <QFileInfo>
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
    roles[TaggableRole] = "taggable";
    roles[FileNameRole] = "fileName";
    roles[TimeRole] = "time";
    roles[GeoPointRole] = "geoPoint";
}

QHash<int,QByteArray> TaggableModel::roleNames() const
{
    return roles;
}

void TaggableModel::addUrls(const QList<QUrl> &urlList)
{
    bool wasEmpty = isEmpty();

    QList<Taggable*> newTaggables;
    foreach (const QUrl &url, urlList) {
        Taggable *taggable = new Taggable(this);
        QObject::connect(taggable, SIGNAL(locationChanged()),
                         this, SLOT(onTaggableChanged()));
        taggable->setFileName(url.toLocalFile());
        if (!taggable->isValid()) {
            delete taggable;
            continue;
        }
        newTaggables.append(taggable);
    }
    int first = rowCount();
    int last = first + newTaggables.count() - 1;
    beginInsertRows(QModelIndex(), first, last);
    taggables.append(newTaggables);
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
        return taggable->time();
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
        Taggable *taggable = taggables.takeAt(row + count - i);
        deleteWhenIdle(taggable);
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

void TaggableModel::onTaggableSaveStateChanged()
{
    Taggable *taggable = qobject_cast<Taggable*>(sender());
    Q_ASSERT(taggable);

    if (taggable->saveState() != Taggable::Saving) {
        busyTaggables.removeOne(taggable);
        delete taggable;
        Q_EMIT busyTaggableCountChanged();
    }
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
        Q_EMIT dataChanged(index(indexMin, 0), index(indexMax, 0));
    }

    checkChangesQueued = false;
    lastChangesTime = Controller::clock();
}

void TaggableModel::deleteWhenIdle(Taggable *taggable)
{
    if (taggable->saveState() == Taggable::Saving) {
        QObject::connect(taggable, SIGNAL(saveStateChanged()),
                         this, SLOT(onTaggableSaveStateChanged()));
        busyTaggables.append(taggable);
        Q_EMIT busyTaggableCountChanged();
    } else {
        delete taggable;
    }
}

TaggableAreaPrivate::TaggableAreaPrivate(TaggableArea *taggableArea):
    model(new TaggableModel(taggableArea)),
    q_ptr(taggableArea)
{
    qmlRegisterType<Mappero::Taggable>();
    qmlRegisterType<Mappero::TaggableModel>();
}

TaggableArea::TaggableArea():
    QQuickItem(0),
    d_ptr(new TaggableAreaPrivate(this))
{
    setFlags(QQuickItem::ItemAcceptsDrops);
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

void TaggableArea::dropEvent(QDropEvent *e)
{
    Q_D(TaggableArea);
    const QMimeData *mimeData = e->mimeData();

    if (mimeData->hasUrls()) {
        d->model->addUrls(mimeData->urls());
    }
}

void TaggableArea::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

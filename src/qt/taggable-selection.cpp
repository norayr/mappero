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

#include "debug.h"
#include "taggable-area.h"
#include "taggable-selection.h"
#include "taggable.h"

using namespace Mappero;

TaggableSelection::TaggableSelection(TaggableModel *model):
    QObject(model),
    _model(model),
    _isEmpty(true),
    _needsSave(false),
    _hasLocation(false),
    _lastIndex(-1)
{
    connect(model,
            SIGNAL(dataChanged(const QModelIndex&,const QModelIndex&)),
            SLOT(onDataChanged(const QModelIndex&,const QModelIndex&)));
    connect(model, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
            this, SLOT(onRowsRemoved(const QModelIndex&,int,int)));
}

void TaggableSelection::selectAll()
{
    int len = _model->rowCount();
    for (int i = 0; i < len; i++) {
        _indexes.insert(i);
    }
    _lastIndex = -1;

    update();
}

void TaggableSelection::setSelection(int index)
{
    _indexes.clear();
    _indexes.insert(index);
    _lastIndex = index;

    update();
}

void TaggableSelection::setShiftSelection(int index)
{
    if (_lastIndex < 0) return setSelection(index);

    int first, last;
    if (_lastIndex > index) {
        first = index;
        last = _lastIndex;
    } else {
        first = _lastIndex;
        last = index;
    }

    for (int i = first; i <= last; i++)
        _indexes.insert(i);
    _lastIndex = index;

    update();
}

void TaggableSelection::setCtrlSelection(int index)
{
    if (_indexes.contains(index)) {
        _indexes.remove(index);
    } else {
        _indexes.insert(index);
    }
    _lastIndex = index;

    update();
}

void TaggableSelection::removeItems()
{
    if (_isEmpty) return;

    QList<int> sortedIndexes = _indexes.toList();
    qSort(sortedIndexes);

    /* Start removing rows from the end, so we don't have to update the indexes
     */
    int last = -1;
    int count = 0;
    int len = sortedIndexes.count();
    for (int i = len - 1; i >= 0; i--) {
        int index = sortedIndexes[i];
        if (index == last - count) {
            count++;
        } else {
            if (last >= 0) {
                _model->removeRows(last - count + 1, count);
            }
            last = index;
            count = 1;
        }
    }
    _model->removeRows(last - count + 1, count);
}

int TaggableSelection::nextUntagged() const
{
    int first = (_lastIndex >= 0) ? _lastIndex + 1 : 0;

    int len = _model->rowCount();
    for (int i = first; i < len; i++) {
        Taggable *taggable = _model->taggable(i);
        if (!taggable->hasLocation()) return i;
    }

    return -1;
}

void TaggableSelection::update()
{
    bool needsSave = false;
    bool hasLocation = false;

    _items.clear();

    foreach (int index, _indexes) {
        Taggable *taggable = _model->taggable(index);

        if (taggable->hasLocation()) hasLocation = true;
        if (taggable->needsSave()) needsSave = true;
        _items.append(taggable);
    }

    if (needsSave != _needsSave) {
        _needsSave = needsSave;
        Q_EMIT needsSaveChanged();
    }

    if (hasLocation != _hasLocation) {
        _hasLocation = hasLocation;
        Q_EMIT hasLocationChanged();
    }

    if (_indexes.isEmpty() != _isEmpty) {
        _isEmpty = _indexes.isEmpty();
        Q_EMIT isEmptyChanged();
    }

    Q_EMIT itemsChanged();
}

void TaggableSelection::onRowsRemoved(const QModelIndex &,
                                      int first, int last)
{
    int count = last - first + 1;

    TaggableIndexes newIndexes;

    foreach (int index, _indexes) {
        if (index < first) {
            newIndexes.insert(index);
        } else if (index > last) {
            newIndexes.insert(index - count);
        }
    }

    _indexes = newIndexes;
    update();
}

void TaggableSelection::onDataChanged(const QModelIndex &,
                                      const QModelIndex &)
{
    update();
}

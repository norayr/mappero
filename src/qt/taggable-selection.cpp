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

    Q_EMIT itemsChanged();
}

void TaggableSelection::onRowsRemoved(const QModelIndex &,
                                      int start, int end)
{
    Q_UNUSED(start);
    Q_UNUSED(end);
    // FIXME TODO
    update();
}

void TaggableSelection::onDataChanged(const QModelIndex &,
                                      const QModelIndex &)
{
    update();
}

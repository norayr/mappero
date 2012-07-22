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

#ifndef MAP_TAGGABLE_SELECTION_H
#define MAP_TAGGABLE_SELECTION_H

#include "taggable.h"

#include <QModelIndex>
#include <QObject>
#include <QSet>

namespace Mappero {

class TaggableModel;

typedef QSet<int> TaggableIndexes;

class TaggableSelection: public QObject
{
    Q_OBJECT
    Q_PROPERTY(Mappero::TaggableModel *model READ model CONSTANT);
    Q_PROPERTY(QList<QObject *> items READ items NOTIFY itemsChanged);
    Q_PROPERTY(bool needsSave READ needsSave NOTIFY needsSaveChanged);
    Q_PROPERTY(bool hasLocation READ hasLocation NOTIFY hasLocationChanged);

public:
    TaggableSelection(TaggableModel *model);
    ~TaggableSelection() {};

    TaggableModel *model() const { return _model; }
    QList<QObject *> items() const { return _items; }
    bool needsSave() const { return _needsSave; }
    bool hasLocation() const { return _hasLocation; }

public Q_SLOTS:
    void setSelection(int index);
    void setShiftSelection(int index);
    void setCtrlSelection(int index);
    bool isSelected(int index) { return _indexes.contains(index); }

Q_SIGNALS:
    void itemsChanged();
    void needsSaveChanged();
    void hasLocationChanged();

private:
    void update();

private Q_SLOTS:
    void onRowsRemoved(const QModelIndex &parent, int start, int end);
    void onDataChanged(const QModelIndex &topLeft,
                       const QModelIndex &bottomRight);

private:
    TaggableModel *_model;
    TaggableIndexes _indexes;
    QList<QObject *> _items;
    bool _needsSave;
    bool _hasLocation;
    int _lastIndex;
};

}; // namespace

#endif /* MAP_TAGGABLE_SELECTION_H */

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

#ifndef MAP_TAGGABLE_AREA_H
#define MAP_TAGGABLE_AREA_H

#include <QAbstractListModel>
#include <QDeclarativeItem>
#include <QUrl>

namespace Mappero {

class Taggable;
class TaggableSelection;

class TaggableModel: public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(Mappero::TaggableSelection *selection READ selection CONSTANT);
    Q_PROPERTY(bool empty READ isEmpty NOTIFY isEmptyChanged);

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
    bool removeRows(int row, int count,
                    const QModelIndex &parent = QModelIndex());

    TaggableSelection *selection() const { return _selection; }
    bool isEmpty() const { return taggables.isEmpty(); }

    Taggable *taggable(int row) const { return taggables[row]; }

Q_SIGNALS:
    void isEmptyChanged();

private Q_SLOTS:
    void onTaggableChanged();
    void checkChanges();

private:
    TaggableSelection *_selection;
    QList<Taggable *> taggables;
    bool checkChangesQueued;
    qint64 lastChangesTime;
};


class TaggableAreaPrivate;
class TaggableArea: public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(Mappero::TaggableModel *model READ model CONSTANT);

public:
    TaggableArea();
    ~TaggableArea();

    TaggableModel *model() const;

protected:
    // reimplemented virtual methods
    void dropEvent(QGraphicsSceneDragDropEvent *event);

private:
    TaggableAreaPrivate *d_ptr;
    Q_DECLARE_PRIVATE(TaggableArea)
};

};

#endif /* MAP_TAGGABLE_AREA_H */

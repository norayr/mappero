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

#ifndef MAP_TAGGABLE_AREA_H
#define MAP_TAGGABLE_AREA_H

#include <QAbstractListModel>
#include <QQuickItem>
#include <QSet>
#include <QUrl>

namespace Mappero {

class Taggable;
class TaggableSelection;

class TaggableModel: public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(Mappero::TaggableSelection *selection READ selection CONSTANT);
    Q_PROPERTY(bool empty READ isEmpty NOTIFY isEmptyChanged);
    Q_PROPERTY(int busyTaggableCount READ busyTaggableCount \
               NOTIFY busyTaggableCountChanged)

public:
    enum TaggableModelRoles {
        TaggableRole = Qt::UserRole + 1,
        FileNameRole,
        TimeRole,
        GeoPointRole,
    };

    TaggableModel(QObject *parent = 0);

    QHash<int,QByteArray> roleNames() const;

    Q_INVOKABLE void addUrls(const QList<QUrl> &urlList);

    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const;
    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const;
    bool removeRows(int row, int count,
                    const QModelIndex &parent = QModelIndex());

    TaggableSelection *selection() const { return _selection; }
    bool isEmpty() const { return taggables.isEmpty(); }

    Taggable *taggable(int row) const { return taggables[row]; }

    int busyTaggableCount() const {
        return busyTaggables.count() + busyRemovedTaggables.count();
    }

Q_SIGNALS:
    void isEmptyChanged();
    void busyTaggableCountChanged();

private Q_SLOTS:
    void onTaggableChanged();
    void onTaggableSaveStateChanged();
    void checkChanges();

private:
    void deleteWhenIdle(Taggable *taggable);

private:
    TaggableSelection *_selection;
    QList<Taggable *> taggables;
    QSet<Taggable *> busyTaggables;
    QSet<Taggable *> busyRemovedTaggables;
    bool checkChangesQueued;
    qint64 lastChangesTime;
    QHash<int, QByteArray> roles;
};


class TaggableAreaPrivate;
class TaggableArea: public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(Mappero::TaggableModel *model READ model CONSTANT);

public:
    TaggableArea();
    ~TaggableArea();

    TaggableModel *model() const;

protected:
    // reimplemented virtual methods
    void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;

private:
    TaggableAreaPrivate *d_ptr;
    Q_DECLARE_PRIVATE(TaggableArea)
};

};

#endif /* MAP_TAGGABLE_AREA_H */

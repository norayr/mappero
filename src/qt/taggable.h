/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2012-2020 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAP_TAGGABLE_H
#define MAP_TAGGABLE_H

#include "types.h"

#include <QDateTime>
#include <QQuickImageProvider>
#include <QMap>
#include <QMetaType>
#include <QObject>
#include <QSize>
#include <QString>
#include <QUrl>

class QPixmap;

namespace Mappero {

class TaggablePrivate;

class Taggable: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl pixmapUrl READ pixmapUrl CONSTANT);
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName \
               NOTIFY fileNameChanged);
    Q_PROPERTY(Mappero::GeoPoint location READ location WRITE setLocation \
               NOTIFY locationChanged);
    Q_PROPERTY(bool hasLocation READ hasLocation NOTIFY locationChanged);
    Q_PROPERTY(SaveState saveState READ saveState NOTIFY saveStateChanged);
    Q_PROPERTY(QDateTime time READ time NOTIFY fileNameChanged);
    Q_ENUMS(SaveState)

public:
    Taggable(QObject *parent = 0);
    ~Taggable();

    enum SaveState {
        Unchanged = 0,
        NeedsSave,
        Saving,
    };

    bool isValid() const;

    QUrl pixmapUrl() const;

    void setFileName(const QString &fileName);
    QString fileName() const;

    void setLocation(const GeoPoint &location);
    Q_INVOKABLE void setCorrelatedLocation(const GeoPoint &location);
    GeoPoint location() const;
    bool hasLocation() const;

    SaveState saveState() const;
    qint64 lastChange() const;

    QDateTime time() const;

    QPixmap pixmap(QSize *size, const QSize &requestedSize) const;

public Q_SLOTS:
    void clearLocation();
    void open() const;
    void reload();
    void save();

Q_SIGNALS:
    void fileNameChanged();
    void locationChanged();
    void saveStateChanged();

public:
    class ImageProvider: public QQuickImageProvider
    {
    public:
        ~ImageProvider();

        static ImageProvider *instance();

        static QString name();
        QPixmap requestPixmap(const QString &id,
                              QSize *size,
                              const QSize &requestedSize);

    private:
        friend class Taggable;
        ImageProvider();
        QMap<QString,Taggable *> taggables;
    };

private:
    TaggablePrivate *d_ptr;
    Q_DECLARE_PRIVATE(Taggable)
};

}; // namespace

Q_DECLARE_METATYPE(Mappero::Taggable*)

#endif /* MAP_TAGGABLE_H */

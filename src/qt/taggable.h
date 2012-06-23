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

#ifndef MAP_TAGGABLE_H
#define MAP_TAGGABLE_H

#include <QDeclarativeImageProvider>
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
    Q_PROPERTY(time_t time READ time CONSTANT);

public:
    Taggable(QObject *parent = 0);
    ~Taggable();

    QUrl pixmapUrl() const;

    void setFileName(const QString &fileName);
    QString fileName() const;

    time_t time() const;

    QPixmap pixmap(QSize *size, const QSize &requestedSize) const;

Q_SIGNALS:
    void fileNameChanged();

public:
    class ImageProvider: public QDeclarativeImageProvider
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

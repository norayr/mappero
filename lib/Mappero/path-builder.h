/*
 * Copyright (C) 2013-2020 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAPPERO_PATH_BUILDER_H
#define MAPPERO_PATH_BUILDER_H

#include "path.h"

#include <QObject>
#include <QPointF>
#include <QVariantMap>

namespace Mappero {

class PathBuilderPrivate;
class MAPPERO_EXPORT PathBuilder: public QObject
{
    Q_OBJECT
    Q_PROPERTY(Mappero::Path path READ path NOTIFY pathChanged)
    Q_PROPERTY(QString source READ source WRITE setSource \
               NOTIFY sourceChanged)

public:
    PathBuilder(QObject *parent = 0);
    ~PathBuilder();

    Path path() const;

    void setSource(const QString &source);
    QString source() const;

    Q_INVOKABLE void clear();
    Q_INVOKABLE void addPoint(const QVariantMap &pointData);

Q_SIGNALS:
    void pathChanged();
    void sourceChanged();

private:
    PathBuilderPrivate *d_ptr;
    Q_DECLARE_PRIVATE(PathBuilder)
};

} // namespace

#endif // MAPPERO_PATH_BUILDER_H

/*
 * Copyright (C) 2019-2020 Alberto Mardegan <mardy@users.sourceforge.net>
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
 * along with Photokinesis.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MARDY_UPDATER_H
#define MARDY_UPDATER_H

#include <QJsonObject>
#include <QObject>
#include <QUrl>

namespace Mardy {

class UpdaterPrivate;
class Updater: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl versionUrl READ versionUrl WRITE setVersionUrl
               NOTIFY versionUrlChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QJsonObject latestVersion READ latestVersion
               NOTIFY finished)

public:
    enum Status {
        Idle = 0,
        Busy,
        Succeeded,
        Failed,
    };
    Q_ENUM(Status)

    Updater(QObject *parent = 0);
    ~Updater();

    void setVersionUrl(const QUrl &url);
    QUrl versionUrl() const;

    Status status() const;

    QJsonObject latestVersion() const;

    Q_INVOKABLE void start();

Q_SIGNALS:
    void versionUrlChanged();
    void statusChanged();

    void finished();

private:
    QScopedPointer<UpdaterPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Updater)
};

} // namespace

#endif // MARDY_UPDATER_H

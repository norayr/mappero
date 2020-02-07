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

#ifndef MAPPERO_PLUGIN_H
#define MAPPERO_PLUGIN_H

#include "global.h"

#include <QObject>
#include <QQmlComponent>
#include <QUrl>
#include <QVariantMap>

namespace Mappero {

#define MAPPERO_PLUGIN_KEY_NAME "id"
#define MAPPERO_PLUGIN_KEY_DISPLAY_NAME "name"
#define MAPPERO_PLUGIN_KEY_ICON "icon"
#define MAPPERO_PLUGIN_KEY_TYPE "type"
#define MAPPERO_PLUGIN_KEY_TRANSLATIONS "translations"

#define MAPPERO_PLUGIN_KEY_BASEDIR "basedir" // for running uninstalled

/* Plugin types */
#define MAPPERO_PLUGIN_TYPE_SEARCH "search"
#define MAPPERO_PLUGIN_TYPE_ROUTING "routing"

class PluginPrivate;
class MAPPERO_EXPORT Plugin: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QUrl icon READ icon CONSTANT)
    Q_PROPERTY(QString type READ type CONSTANT)
    Q_PROPERTY(QString translations READ translations CONSTANT)

public:
    Plugin(const QVariantMap &manifestData, QObject *parent = 0);
    ~Plugin();

    static QString name(const QVariantMap &manifestData);
    static QString displayName(const QVariantMap &manifestData);
    static QUrl icon(const QVariantMap &manifestData);
    static QString type(const QVariantMap &manifestData);
    static QString translations(const QVariantMap &manifestData);

    QString name() const;
    QString displayName() const;
    QUrl icon() const;
    QString type() const;
    QString translations() const;

protected:
    const QVariantMap &manifestData() const;
    QUrl resolvePath(const QString &path) const;
    QQmlComponent *loadComponent(const QString &key);

private:
    PluginPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Plugin)
};

} // namespace

#endif // MAPPERO_PLUGIN_H

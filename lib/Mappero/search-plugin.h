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

#ifndef MAPPERO_SEARCH_PLUGIN_H
#define MAPPERO_SEARCH_PLUGIN_H

#include "plugin.h"

#include "types.h"

#include <QObject>
#include <QPointF>
#include <QVariant>

namespace Mappero {

#define MAPPERO_SEARCH_PLUGIN_KEY_DELEGATE "delegate"
#define MAPPERO_SEARCH_PLUGIN_KEY_MODEL "model"

class SearchPluginPrivate;
class MAPPERO_EXPORT SearchPlugin: public Plugin
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *delegate READ delegate CONSTANT)
    Q_PROPERTY(QVariant model READ model CONSTANT)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(GeoPoint location READ location WRITE setLocation \
               NOTIFY locationChanged)
    Q_PROPERTY(bool isRunning READ isRunning WRITE setRunning \
               NOTIFY isRunningChanged)
    Q_PROPERTY(int itemsPerPage READ itemsPerPage WRITE setItemsPerPage \
               NOTIFY itemsPerPageChanged)

public:
    SearchPlugin(const QVariantMap &manifestData, QObject *parent = 0);
    ~SearchPlugin();

    virtual QQmlComponent *delegate();
    virtual QVariant model();

    void setQuery(const QString &query);
    QString query() const;

    void setLocation(const GeoPoint &location);
    GeoPoint location() const;

    void setRunning(bool isRunning);
    bool isRunning() const;

    void setItemsPerPage(int itemsPerPage);
    int itemsPerPage() const;

Q_SIGNALS:
    void queryChanged();
    void locationChanged();
    void isRunningChanged();
    void itemsPerPageChanged();

private:
    SearchPluginPrivate *d_ptr;
    Q_DECLARE_PRIVATE(SearchPlugin)
};

} // namespace

#endif // MAPPERO_SEARCH_PLUGIN_H

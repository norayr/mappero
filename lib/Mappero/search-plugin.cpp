/*
 * Copyright (C) 2013 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include "qml-search-model.h"
#include "search-plugin.h"

#include <QQmlContext>
#include <QQmlEngine>

using namespace Mappero;

#define DEFAULT_DELEGATE "qrc:/Mappero/Plugin/DefaultResultsDelegate.qml"

namespace Mappero {

class SearchPluginPrivate
{
    Q_DECLARE_PUBLIC(SearchPlugin)

    inline SearchPluginPrivate(SearchPlugin *q);
    ~SearchPluginPrivate() {};

private:
    mutable SearchPlugin *q_ptr;
    QString m_query;
    QPointF m_location;
    bool m_isRunning;
    int m_itemsPerPage;
};

} // namespace

SearchPluginPrivate::SearchPluginPrivate(SearchPlugin *q):
    q_ptr(q)
{
}

SearchPlugin::SearchPlugin(const QVariantMap &manifestData, QObject *parent):
    Plugin(manifestData, parent),
    d_ptr(new SearchPluginPrivate(this))
{
}

SearchPlugin::~SearchPlugin()
{
    delete d_ptr;
}

QQmlComponent *SearchPlugin::delegate()
{
    static bool pathAdded = false;

    if (!pathAdded) {
        QQmlContext *context = QQmlEngine::contextForObject(this);
        context->engine()->addImportPath("qrc:/");
        pathAdded = true;
    }

    /* The default implementation loads the delegate defined in the manifest
     * file, if any. */
    static const QString key =
        QStringLiteral(MAPPERO_SEARCH_PLUGIN_KEY_DELEGATE);
    if (manifestData().contains(key)) {
        return loadComponent(key);
    } else {
        QQmlContext *context = QQmlEngine::contextForObject(this);
        if (Q_UNLIKELY(context == 0)) return 0;

        return new QQmlComponent(context->engine(),
                                 QUrl(QStringLiteral(DEFAULT_DELEGATE)),
                                 this);
    }
}

QVariant SearchPlugin::model()
{
    /* The default implementation loads a component defined in the manifest and
     * instantiates it. */
    static const QString key =
        QStringLiteral(MAPPERO_SEARCH_PLUGIN_KEY_MODEL);

    QQmlComponent *modelComponent = loadComponent(key);
    if (!modelComponent) return QVariant();
    if (modelComponent->isError()) {
        DEBUG() << "errors" << modelComponent->errors();
    }

    QQmlContext *context =
        new QQmlContext(QQmlEngine::contextForObject(this), this);
    context->setContextProperty(QStringLiteral("search"), this);
    QObject *model = modelComponent->create(context);
    return QVariant::fromValue<QObject*>(model);
}

void SearchPlugin::setQuery(const QString &query)
{
    Q_D(SearchPlugin);
    if (query == d->m_query) return;
    d->m_query = query;
    Q_EMIT queryChanged();
}

QString SearchPlugin::query() const
{
    Q_D(const SearchPlugin);
    return d->m_query;
}

void SearchPlugin::setLocation(const QPointF &location)
{
    Q_D(SearchPlugin);
    if (location == d->m_location) return;
    d->m_location = location;
    Q_EMIT locationChanged();
}

QPointF SearchPlugin::location() const
{
    Q_D(const SearchPlugin);
    return d->m_location;
}

void SearchPlugin::setRunning(bool isRunning)
{
    Q_D(SearchPlugin);
    if (isRunning == d->m_isRunning) return;
    d->m_isRunning = isRunning;
    Q_EMIT isRunningChanged();
}

bool SearchPlugin::isRunning() const
{
    Q_D(const SearchPlugin);
    return d->m_isRunning;
}

void SearchPlugin::setItemsPerPage(int itemsPerPage)
{
    Q_D(SearchPlugin);
    if (itemsPerPage == d->m_itemsPerPage) return;
    d->m_itemsPerPage = itemsPerPage;
    Q_EMIT itemsPerPageChanged();
}

int SearchPlugin::itemsPerPage() const
{
    Q_D(const SearchPlugin);
    return d->m_itemsPerPage;
}

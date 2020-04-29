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

#include "updater.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSysInfo>
#include <QVersionNumber>
#include <QtQml>

using namespace Mardy;

namespace Mardy {

class UpdaterPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Updater)

public:
    UpdaterPrivate(Updater *q);

    void ensureUserAgent();
    void setStatus(Updater::Status status);

    void start();

    void update();

private:
    QUrl m_versionUrl;
    QString m_userAgent;
    Updater::Status m_status;
    QJsonObject m_latestVersion;
    Updater *q_ptr;
};

} // namespace

UpdaterPrivate::UpdaterPrivate(Updater *q):
    QObject(),
    m_status(Updater::Idle),
    q_ptr(q)
{
}

void UpdaterPrivate::ensureUserAgent()
{
    if (!m_userAgent.isEmpty()) return;

    m_userAgent = QString("%1/%2 (%3/%4 %5; %6)")
        .arg(QCoreApplication::applicationName())
        .arg(QCoreApplication::applicationVersion())
        .arg(QSysInfo::productType())
        .arg(QSysInfo::productVersion())
        .arg(QSysInfo::currentCpuArchitecture())
        .arg(QLocale().name());
}

void UpdaterPrivate::setStatus(Updater::Status status)
{
    Q_Q(Updater);
    if (status != m_status) {
        m_status = status;
        Q_EMIT q->statusChanged();
    }
}

void UpdaterPrivate::start()
{
    Q_Q(Updater);
    setStatus(Updater::Busy);

    QVersionNumber ourVersion =
        QVersionNumber::fromString(QCoreApplication::applicationVersion());
    QQmlEngine *engine = qmlEngine(q);
    Q_ASSERT(engine);

    QNetworkAccessManager *nam = engine->networkAccessManager();
    Q_ASSERT(nam);

    QNetworkRequest req(m_versionUrl);
    ensureUserAgent();
    req.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);

    QNetworkReply *reply = nam->get(req);
    QObject::connect(reply, &QNetworkReply::finished,
                     this, [this, q, reply, ourVersion]() {
        reply->deleteLater();

        QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
        QString osName =
#if defined Q_OS_MACOS
            "macos"
#elif defined Q_OS_WIN
            "windows"
#else
            "linux"
#endif
            ;
        const QJsonObject o = json.object();
        QString latestVersion = o[osName].toString();
        if (latestVersion.isEmpty()) {
            setStatus(Updater::Failed);
        } else {
            QVersionNumber latest = QVersionNumber::fromString(latestVersion);
            m_latestVersion = {
                { "version", latestVersion },
                { "isNewer", (latest > ourVersion) },
            };
            setStatus(Updater::Succeeded);
        }
        Q_EMIT q->finished();
    });
}

Updater::Updater(QObject *parent):
    QObject(parent),
    d_ptr(new UpdaterPrivate(this))
{
}

Updater::~Updater()
{
}

void Updater::setVersionUrl(const QUrl &url)
{
    Q_D(Updater);
    d->m_versionUrl = url;
    Q_EMIT versionUrlChanged();
}

QUrl Updater::versionUrl() const
{
    Q_D(const Updater);
    return d->m_versionUrl;
}

Updater::Status Updater::status() const
{
    Q_D(const Updater);
    return d->m_status;
}

QJsonObject Updater::latestVersion() const
{
    Q_D(const Updater);
    return d->m_latestVersion;
}

void Updater::start()
{
    Q_D(Updater);
    d->start();
}

#include "updater.moc"

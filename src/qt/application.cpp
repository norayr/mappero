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
 * along with Mappero.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "application.h"

#include <QByteArray>
#include <QDebug>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QIcon>
#include <QMetaObject>
#include <QThreadPool>
#ifdef Q_OS_ANDROID
#include <QSvgRenderer>
#endif

using namespace Mappero;

namespace Mappero {

class ApplicationPrivate
{
    Q_DECLARE_PUBLIC(Application)

    enum Mode {
        Mapping = 0,
        Geotagger,
    };

public:
    ApplicationPrivate(Application *q);

    void parseCmdLine();

private:
    Mode m_mode;
    Application *q_ptr;
};

} // namespace

ApplicationPrivate::ApplicationPrivate(Application *q):
#ifdef Q_OS_MACOS
    m_mode(Geotagger),
#else
    m_mode(Mapping),
#endif
    q_ptr(q)
{
    QString applicationDir = QCoreApplication::applicationDirPath();
    if (applicationDir.endsWith("/bin")) {
        QString dataDir = applicationDir.left(applicationDir.length() - 3) +
            "share";
        QByteArray originalPath = qgetenv("XDG_DATA_DIRS");
        qputenv("XDG_DATA_DIRS", dataDir.toUtf8() + ':' + originalPath);
    }

    parseCmdLine();
}

void ApplicationPrivate::parseCmdLine()
{
    Q_Q(Application);

    enum Status { Start, Items, };

    QList<QUrl> items;

    const QStringList args = q->arguments().mid(1);
    Status status = Start;
    for (const QString &arg: args) {
        if (status == Start) {
            if (arg[0] == '-') {
                if (arg == "--geotag") {
                    m_mode = Geotagger;
                } else if (arg == "--") {
                    status = Items;
                } else {
                    qWarning() << "Unknown option" << arg;
                    status = Items;
                }
                continue;
            } else {
                status = Items;
            }
        }

        if (status == Items) {
            QFileInfo fileInfo(arg);
            if (fileInfo.isFile()) {
                items.append(QUrl::fromLocalFile(arg));
            }
        }
    }

    if (!items.isEmpty()) {
        QMetaObject::invokeMethod(q, "itemsAddRequest", Qt::QueuedConnection,
                                  Q_ARG(QList<QUrl>, items));
    }
}

Application::Application(int &argc, char **argv):
#ifdef QT_WIDGETS_LIB
    QApplication(argc, argv),
#else
    QGuiApplication(argc, argv),
#endif
    d_ptr(new ApplicationPrivate(this))
{
    Q_D(const Application);
    setOrganizationName("mardy.it");
    setApplicationName("mappero");
    setApplicationVersion(MAPPERO_VERSION);
    if (d->m_mode == ApplicationPrivate::Geotagger) {
        setApplicationDisplayName("Mappero Geotagger");
        setWindowIcon(QIcon(":/mappero-geotagger"));
    } else {
        setApplicationDisplayName("Mappero");
    }
}

Application::~Application()
{
    QThreadPool::globalInstance()->waitForDone(5000);
#ifdef Q_OS_ANDROID
    QSvgRenderer unusedRenderer; // Just to force linking to SVG
#endif
}

QString Application::firstPage() const
{
    Q_D(const Application);
    return d->m_mode == ApplicationPrivate::Mapping ?
        "MainPage.qml" : "GeoTagPage.qml";
}

bool Application::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
        Q_EMIT itemsAddRequest({ openEvent->url() });
    }

    return QGuiApplication::event(event);
}

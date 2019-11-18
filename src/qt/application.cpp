/*
 * Copyright (C) 2019 Alberto Mardegan <mardy@users.sourceforge.net>
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

#include <QDebug>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QMetaObject>
#include <QThreadPool>

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
    setOrganizationName("mardy.it");
    setApplicationName("mappero");
    setApplicationVersion(MAPPERO_VERSION);
}

Application::~Application()
{
    QThreadPool::globalInstance()->waitForDone(5000);
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

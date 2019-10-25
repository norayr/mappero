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
#include <QFileOpenEvent>
#include <QThreadPool>

using namespace Mappero;

namespace Mappero {

class ApplicationPrivate
{
    Q_DECLARE_PUBLIC(Application)

public:
    ApplicationPrivate(Application *q);

private:
    Application *q_ptr;
};

} // namespace

ApplicationPrivate::ApplicationPrivate(Application *q):
    q_ptr(q)
{
}

Application::Application(int &argc, char **argv):
    QGuiApplication(argc, argv),
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
    QString pageName = "MainPage.qml";
#ifdef Q_OS_OSX
    if (1) {
#else
    if (arguments().contains("--geotag")) {
#endif
        pageName = "GeoTagPage.qml";
    }

    return pageName;
}

bool Application::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
        Q_EMIT itemsAddRequest({ openEvent->url() });
    }

    return QGuiApplication::event(event);
}

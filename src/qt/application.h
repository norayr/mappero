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

#ifndef MAPPERO_APPLICATION_H
#define MAPPERO_APPLICATION_H

#include <QGuiApplication>
#include <QString>

namespace Mappero {

class ApplicationPrivate;
class Application: public QGuiApplication
{
    Q_OBJECT
    Q_PROPERTY(QString firstPage READ firstPage CONSTANT)

public:
    Application(int &argc, char **argv);
    ~Application();

    QString firstPage() const;

private:
    QScopedPointer<ApplicationPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Application)
};

} // namespace

#endif // MAPPERO_APPLICATION_H

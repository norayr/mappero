/* vi: set et sw=4 ts=8 cino=t0,(0: */
/*
 * Copyright (C) 2012 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "configuration.h"
#include "configuration.h.moc"
#include "debug.h"

using namespace Mappero;

namespace Mappero {
class ConfigurationPrivate
{
    Q_DECLARE_PUBLIC(Configuration)

    ConfigurationPrivate(Configuration *configuration):
        q_ptr(configuration)
    {
    }

    ~ConfigurationPrivate()
    {
    }

private:
    mutable Configuration *q_ptr;
};
};

Configuration::Configuration(QObject *parent):
    QSettings(parent),
    d_ptr(new ConfigurationPrivate(this))
{
}

Configuration::~Configuration()
{
    delete d_ptr;
}


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

#ifndef MAPPEROUI_GLOBAL_H
#define MAPPEROUI_GLOBAL_H

#include <QtCore/QtGlobal>

#if defined(BUILDING_LIBMAPPEROUI)
#  define MAPPEROUI_EXPORT Q_DECL_EXPORT
#else
#  define MAPPEROUI_EXPORT Q_DECL_IMPORT
#endif

#endif /* MAPPEROUI_GLOBAL_H */


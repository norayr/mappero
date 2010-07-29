/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAP_GNUITE_H
#define MAP_GNUITE_H

#include <mappero/router.h>
 
G_BEGIN_DECLS

#define MAP_TYPE_GNUITE         (map_gnuite_get_type())
#define MAP_GNUITE(o)           (G_TYPE_CHECK_INSTANCE_CAST((o), MAP_TYPE_GNUITE, MapGnuite))
#define MAP_GNUITE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MAP_TYPE_GNUITE, MapGnuiteClass))
#define MAP_IS_GNUITE(o)        (G_TYPE_CHECK_INSTANCE_TYPE((o), MAP_TYPE_GNUITE))
#define MAP_IS_GNUITE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE((k), MAP_TYPE_GNUITE))
#define MAP_GNUITE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), MAP_TYPE_GNUITE, MapGnuiteClass))

typedef struct _MapGnuite MapGnuite;
typedef struct _MapGnuiteClass MapGnuiteClass;


struct _MapGnuite
{
    GObject parent;
    gboolean avoid_highways;
};

struct _MapGnuiteClass
{
    GObjectClass parent;
};

GType map_gnuite_get_type(void) G_GNUC_CONST;

G_END_DECLS
#endif /* MAP_GNUITE_H */

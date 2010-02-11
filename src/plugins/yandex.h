/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This file is part of Maemo Mapper.
 *
 * Maemo Mapper is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Maemo Mapper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Maemo Mapper.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAP_YANDEX_H
#define MAP_YANDEX_H

#include <router.h>
 
G_BEGIN_DECLS

#define MAP_TYPE_YANDEX         (map_yandex_get_type())
#define MAP_YANDEX(o)           (G_TYPE_CHECK_INSTANCE_CAST((o), MAP_TYPE_YANDEX, MapYandex))
#define MAP_YANDEX_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MAP_TYPE_YANDEX, MapYandexClass))
#define MAP_IS_YANDEX(o)        (G_TYPE_CHECK_INSTANCE_TYPE((o), MAP_TYPE_YANDEX))
#define MAP_IS_YANDEX_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE((k), MAP_TYPE_YANDEX))
#define MAP_YANDEX_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), MAP_TYPE_YANDEX, MapYandexClass))

typedef struct _MapYandex MapYandex;
typedef struct _MapYandexClass MapYandexClass;


struct _MapYandex
{
    GObject parent;
};

struct _MapYandexClass
{
    GObjectClass parent;
};

GType map_yandex_get_type(void) G_GNUC_CONST;

G_END_DECLS
#endif /* MAP_YANDEX_H */

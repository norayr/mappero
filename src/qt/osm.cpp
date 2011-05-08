/* vi: set et sw=4 ts=8 cino=t0,(0: */
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
#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "osm.h"

#include "controller.h"
#include "debug.h"
#include "view.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>

/* number of buttons per column */
#define N_BUTTONS_COLUMN   4

/* number of buttons on the bottom row */
#define N_BUTTONS_BOTTOM    2

/* total number of buttons: 2 columns */
#define N_BUTTONS   (N_BUTTONS_COLUMN * 2 + N_BUTTONS_BOTTOM)

/* button indices */
#define IDX_BUTTON0_BOTTOM  (N_BUTTONS_COLUMN * 2)

#define BUTTON_SIZE_X   72
#define BUTTON_SIZE_Y   72
#define BUTTON_BORDER_OFFSET    8
#define BUTTON_X_OFFSET ((BUTTON_SIZE_X / 2) + BUTTON_BORDER_OFFSET)
#define BUTTON_Y_OFFSET ((BUTTON_SIZE_Y / 2) + BUTTON_BORDER_OFFSET)

using namespace Mappero;

class OsmButton: public QGraphicsPixmapItem
{
public:
    enum Action {
        NONE,
        POINT,
        PATH,
        ROUTE,
        GO_TO,
        ZOOM_IN,
        ZOOM_OUT,
        FULLSCREEN,
        SETTINGS,
        GPS_TOGGLE,
    };

    OsmButton(const QString &fileName, Action action, QGraphicsItem *parent):
        QGraphicsPixmapItem(QPixmap(fileName), parent)
    {
        this->action = action;
        setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event)
    {
        event->accept();
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
    {
        if (!contains(event->pos())) return;

        switch (action) {
        case FULLSCREEN:
            Controller::instance()->view()->switchFullscreen();
            break;
        }
    }

private:
    Action action;
};

struct ButtonData {
    const char *icon;
    OsmButton::Action action;
};

static const ButtonData buttonData[N_BUTTONS] = {
    /* column 1 */
    { "maemo-mapper-point", OsmButton::POINT },
    { "maemo-mapper-path", OsmButton::PATH },
    { "maemo-mapper-route", OsmButton::ROUTE },
    { "maemo-mapper-go-to", OsmButton::GO_TO },
    /* column 2 */
    { "maemo-mapper-zoom-in", OsmButton::ZOOM_IN },
    { "maemo-mapper-zoom-out", OsmButton::ZOOM_OUT },
    { NULL, OsmButton::NONE },
    { "maemo-mapper-fullscreen", OsmButton::FULLSCREEN },
    /* bottom row */
    { "maemo-mapper-settings", OsmButton::SETTINGS },
    { "maemo-mapper-gps-disable", OsmButton::GPS_TOGGLE },
};

Osm::Osm()
{
    setFlags(QGraphicsItem::ItemHasNoContents);
    QString base = QString("/usr/share/icons/hicolor/scalable/hildon/");
    for (int i = 0; i < N_BUTTONS; i++) {
        new OsmButton(base + buttonData[i].icon + ".png", buttonData[i].action,
                      this);
    }
}

void Osm::setSize(const QSize &size)
{
    int width = size.width();
    int height = size.height();

    int col, i, x, y, dy, dx;
    int n_columns, n_buttons_column;

    /* lay out the buttons according to the new screen size */
    if (width > height)
    {
        n_buttons_column = N_BUTTONS_COLUMN;
        n_columns = 2;
    }
    else
    {
        n_buttons_column = N_BUTTONS_COLUMN * 2;
        n_columns = 1;
    }

    dy = height / n_buttons_column;
    x = BUTTON_X_OFFSET;
    for (col = 0; col < n_columns; col++)
    {
        y = dy / 2;

        if (col == 1)
        {
            x = width - BUTTON_X_OFFSET;
        }

        for (i = 0; i < n_buttons_column; i++)
        {
            QGraphicsItem *button = childItems()[col * N_BUTTONS_COLUMN + i];

            if (button)
                button->setPos(x - BUTTON_SIZE_X / 2, y - BUTTON_SIZE_Y / 2);
            y += dy;
        }
    }

    /* bottom row */
    y = height - BUTTON_Y_OFFSET;
    dx = dy; /* keep the same distance that we used for columns */
    x = (width - dx * (N_BUTTONS_BOTTOM - 1)) / 2;
    for (i = 0; i < N_BUTTONS_BOTTOM; i++)
    {
        QGraphicsItem *button = childItems()[IDX_BUTTON0_BOTTOM + i];

        if (button)
            button->setPos(x - BUTTON_SIZE_X / 2, y - BUTTON_SIZE_Y / 2);
        x += dx;
    }
}


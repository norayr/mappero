/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
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

#ifndef MAPPERO_KML_H
#define MAPPERO_KML_H

#include "path.h"

namespace Mappero {

class Kml: public PathStream
{
public:
    Kml();
    ~Kml();

    bool read(QXmlStreamReader &xml, PathData &pathData);
    bool write(QXmlStreamWriter &xml, const PathData &pathData);

private:
    void parseCoordinates(QXmlStreamReader &xml);
    void parseLineString(QXmlStreamReader &xml);
    void parsePoint(QXmlStreamReader &xml);
    void parseGeometryCollection(QXmlStreamReader &xml);

private:
    QVector<PathPoint> points;
};

}; // namespace

#endif /* MAPPERO_KML_H */
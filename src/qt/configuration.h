/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2012-2020 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAP_CONFIGURATION_H
#define MAP_CONFIGURATION_H

#include <Mappero/types.h>

#include <QObject>
#include <QSettings>

namespace Mappero {

class ConfigurationPrivate;
class Configuration: public QSettings
{
    Q_OBJECT
    Q_PROPERTY(Mappero::GeoPoint lastPosition READ lastPosition \
               WRITE setLastPosition NOTIFY lastPositionChanged);
    Q_PROPERTY(qreal lastZoomLevel READ lastZoomLevel \
               WRITE setLastZoomLevel NOTIFY lastZoomLevelChanged);
    Q_PROPERTY(QString lastMainLayer READ lastMainLayer \
               WRITE setLastMainLayer NOTIFY lastMainLayerChanged);
    Q_PROPERTY(int gpsInterval READ gpsInterval \
               WRITE setGpsInterval NOTIFY gpsIntervalChanged);
    Q_PROPERTY(QString preferredSearchPlugin READ preferredSearchPlugin
               WRITE setPreferredSearchPlugin
               NOTIFY preferredSearchPluginChanged)

public:
    Configuration(QObject *parent = 0);
    ~Configuration();

    QString mapCacheDir() const;

    void setLastPosition(const GeoPoint &position);
    GeoPoint lastPosition() const;

    void setLastZoomLevel(qreal zoom);
    qreal lastZoomLevel() const;

    void setLastMainLayer(const QString &layerName);
    QString lastMainLayer() const;

    void setGpsInterval(int interval);
    int gpsInterval() const;

    void setPreferredSearchPlugin(const QString &pluginName);
    QString preferredSearchPlugin() const;

Q_SIGNALS:
    void lastPositionChanged();
    void lastZoomLevelChanged();
    void lastMainLayerChanged();
    void gpsIntervalChanged();
    void preferredSearchPluginChanged();

private:
    ConfigurationPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Configuration)
};

};

#endif /* MAP_CONFIGURATION_H */

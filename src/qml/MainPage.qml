import QtQuick 1.0
import Mappero 1.0

Item {
    anchors.fill: parent

    Map {
        id: map
        anchors.fill: parent
        flickable: mapFlickable

        mainLayerId: "OpenStreetMap I"
        center: Mappero.conf.lastPosition
        requestedZoomLevel: Mappero.conf.lastZoomLevel
        followGps: visible

        PathLayer {
            Tracker {
                id: tracker
                tracking: true
                color: "red"
            }
        }
    }

    MapFlickable {
        id: mapFlickable
        anchors.fill: parent
    }

    Osm {
        id: osm
        anchors.fill: parent
    }

    Component.onDestruction: {
        Mappero.conf.lastPosition = map.center
        Mappero.conf.lastZoomLevel = map.zoomLevel
    }
}

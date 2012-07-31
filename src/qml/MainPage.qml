import QtQuick 1.0
import Mappero 1.0

Item {
    anchors.fill: parent

    Tracker {
        id: tracker
        tracking: true
    }

    Map {
        id: map
        anchors.fill: parent
        flickable: mapFlickable

        mainLayerId: "OpenStreetMap I"
        center: Mappero.conf.lastPosition
        requestedZoomLevel: Mappero.conf.lastZoomLevel
        followGps: visible

        PathItem {
            tracker: tracker
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

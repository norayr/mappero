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

        tracker: tracker
        mainLayerId: "OpenStreetMap I"
        center: Qt.point(60.19997, 24.94057)
        requestedZoomLevel: 8
        followGps: visible
    }

    MapFlickable {
        id: mapFlickable
        anchors.fill: parent
    }

    Osm {
        id: osm
        anchors.fill: parent
    }
}

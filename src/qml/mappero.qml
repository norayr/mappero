import QtQuick 1.0
import Mappero 1.0

Rectangle {
    id: screen
    width: 800
    height: 480

    Osm {
        id: osm
        z: 1
        anchors.fill: parent
    }

    Map {
        id: map
        anchors.fill: parent
        flickable: mapFlickable

        mainLayerId: "OpenStreetMap I"
        center: Qt.point(60.19997, 24.94057)
        requestedZoomLevel: 8
        followGps: true
    }

    MapFlickable {
        id: mapFlickable
        anchors.fill: parent
    }
}

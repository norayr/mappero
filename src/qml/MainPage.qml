import QtQuick 1.0
import Mappero 1.0

Item {
    anchors.fill: parent
    property bool __isPage: true

    LayerManager {
        id: layerManager
    }

    Map {
        id: map
        anchors.fill: parent
        flickable: mapFlickable

        mainLayer: layerManager.mainLayer
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

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 4
        color: "white"
        opacity: 0.8
        width: 100
        height: content.height + 8
        visible: !tracker.empty

        Column {
            id: content
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 4
            Text {
                text: Mappero.formatLength(tracker.length)
            }
        }
    }

    Osm {
        id: osm
        anchors.fill: parent
        tracker: tracker
    }

    Connections {
        target: view
        onClosing: {
            Mappero.conf.lastPosition = map.center
            Mappero.conf.lastZoomLevel = map.zoomLevel
            Mappero.conf.gpsInterval = gps.updateInterval
        }
    }
}

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
        pan: Qt.point(flickable.contentX - flickable.neutralX,
                      flickable.contentY - flickable.neutralY)
        flickable: flickable

        mainLayerId: "OpenStreetMap I"
        center: Qt.point(60.19997, 24.94057)
        requestedZoomLevel: 8
        animatedZoomLevel: requestedZoomLevel

        Behavior on animatedZoomLevel {
            SmoothedAnimation {
                target: map
                property: "animatedZoomLevel"
                duration: 500
                velocity: 0.5
            }
        }
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        flickableDirection: Flickable.HorizontalAndVerticalFlick
        property double neutralX: 400000
        property double neutralY: 400000
        contentWidth: neutralX * 2
        contentHeight: neutralY * 2
        contentX: neutralX
        contentY: neutralY

        onMovementEnded: {
            map.panFinished()
            contentX = neutralX
            contentY = neutralY
        }
    }
}

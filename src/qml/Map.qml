import QtQuick 2.0
import Mappero 1.0

MapView {
    animatedZoomLevel: requestedZoomLevel
    animatedCenterUnits: requestedCenterUnits

    Behavior on animatedZoomLevel {
        SmoothedAnimation {
            target: map
            property: "animatedZoomLevel"
            duration: 500
            velocity: 0.5
        }
    }

    Behavior on animatedCenterUnits {
        PropertyAnimation {
            easing.type: Easing.InOutCubic
            duration: 500
        }
    }

    Rectangle {
        anchors.fill: noticeLabel
        opacity: 0.6
        color: "#fff"
    }

    Text {
        id: noticeLabel
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        text: mainLayer.notice
    }
}

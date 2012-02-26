import QtQuick 1.0
import Mappero 1.0

MapItem {
    id: mapItem

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
}

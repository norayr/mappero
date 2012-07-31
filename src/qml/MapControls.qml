import QtQuick 1.0
import "UIConstants.js" as UI

Column {
    property variant map

    width: 32

    ImageButton {
        width: parent.width
        height: width
        enabled: map.minZoomLevel < map.requestedZoomLevel
        source: ":osm-zoom-in"
        onClicked: map.requestedZoomLevel--
    }

    ImageButton {
        width: parent.width
        height: width
        enabled: map.maxZoomLevel > map.requestedZoomLevel
        source: ":osm-zoom-out"
        onClicked: map.requestedZoomLevel++
    }
}

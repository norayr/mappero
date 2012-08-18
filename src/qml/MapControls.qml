import QtQuick 1.0
import "UIConstants.js" as UI

Item {
    id: root
    property variant map
    width: 32 + UI.ToolbarMargins * 2
    height: col.height + UI.ToolbarMargins * 2

    PaneBackground {}

    Column {
        id: col
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: spacing
        spacing: UI.ToolbarMargins

        ImageButton {
            width: parent.width
            height: width
            enabled: root.map.minZoomLevel < root.map.requestedZoomLevel
            source: ":geo-osm-zoom-in"
            onClicked: root.map.requestedZoomLevel--
        }

        ImageButton {
            width: parent.width
            height: width
            enabled: root.map.maxZoomLevel > root.map.requestedZoomLevel
            source: ":geo-osm-zoom-out"
            onClicked: root.map.requestedZoomLevel++
        }
    }
}

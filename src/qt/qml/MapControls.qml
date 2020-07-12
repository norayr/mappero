import Mappero.Ui 1.0
import QtQuick 2.0
import "UIConstants.js" as UI

Item {
    id: root
    property var map
    property alias layerManager: layerSelector.manager
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
            source: "qrc:geo-osm-zoom-in"
            onClicked: root.map.requestedZoomLevel--
        }

        ImageButton {
            width: parent.width
            height: width
            enabled: root.map.maxZoomLevel > root.map.requestedZoomLevel
            source: "qrc:geo-osm-zoom-out"
            onClicked: root.map.requestedZoomLevel++
        }

        ImageButton {
            id: ls
            width: parent.width
            height: width
            source: "qrc:layer-selector"
            onClicked: {
                if (layerSelector.isOpen) {
                    layerSelector.close()
                } else {
                    layerSelector.open()
                }
            }
        }
    }

    LayerSelector {
        id: layerSelector
        source: ls
        position: "left"
    }
}

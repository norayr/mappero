import QtQuick 1.0
import Mappero 1.0
import "UIConstants.js" as UI

Item {
    anchors.fill: parent

    Item {
        id: mapView
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: tagPane.top
        Map {
            id: map
            anchors.fill: parent
            flickable: mapFlickable

            mainLayerId: "OpenStreetMap I"
            center: Qt.point(60.19997, 24.94057)
            requestedZoomLevel: 8
        }

        MapFlickable {
            id: mapFlickable
            anchors.fill: parent
        }
    }

    Rectangle {
        id: tagPane
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: UI.TagPaneHeight
        color: "white"

        TaggableArea {
            id: dropArea
            anchors.fill: parent
        }

        ListView {
            id: taggableView
            anchors.fill: parent
            orientation: ListView.Horizontal
            model: dropArea.model
            spacing: 2
            delegate: TaggableDelegate {
                height: taggableView.height
                width: height
                source: taggable.pixmapUrl
                topText: model.fileName
                bottomText: Qt.formatDateTime(model.time, "d/M/yyyy hh:mm")
            }
        }
    }
}

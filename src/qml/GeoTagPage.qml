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

        MapFlickable {
            id: mapFlickable
            anchors.fill: parent
        }

        Map {
            id: map
            anchors.fill: parent
            flickable: mapFlickable

            mainLayerId: "OpenStreetMap I"
            center: Mappero.geo(59.935, 30.3286)
            requestedZoomLevel: 8

            PoiView {
                id: poiView
                anchors.fill: parent
                model: dropArea.model
                delegate: ImagePoi {
                    width: 80
                    height: 80
                    source: taggable.pixmapUrl
                    topText: model.fileName

                    onDragFinished: {
                        taggable.location = view.itemPos(index)
                    }
                }
            }
        }

        ToolBar {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: UI.ToolSpacing
            anchors.horizontalCenter: parent.horizontalCenter
            selectedItems: taggableView.selectedItems

            onSelectedItemsChanged: console.log("Items: " + selectedItems)
            onGeoSetterDropped: console.log("Dropped on " + pos.x + "," + pos.y)
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

        TaggableView {
            id: taggableView
            anchors.fill: parent
            model: dropArea.model

            onSelectedItemsChanged: {
                // TODO: show all the items on the map?
                if (selectedItems.length == 1) {
                    var taggable = selectedItems[0]
                    if (taggable.hasLocation) {
                        map.requestedCenter = taggable.location
                    }
                }
            }
        }
    }

    Connections {
        target: dropArea.model
        onRowsInserted: map.lookAt(poiView.itemArea, 0, 80, 40)
    }
}

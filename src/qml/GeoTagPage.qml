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
                currentIndex: taggableView.currentIndex
                delegate: ImagePoi {
                    width: 80
                    height: 80
                    source: taggable.pixmapUrl
                    topText: model.fileName

                    onDragFinished: {
                        taggable.location = view.itemPos(index)
                    }

                    onPressed: view.setCurrent(index)
                }

                function setCurrent(index) {
                    taggableView.currentIndex = index
                    dropArea.model.selection.setSelection(index)
                }
            }
        }

        ToolBar {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: UI.ToolSpacing
            anchors.horizontalCenter: parent.horizontalCenter
            selection: dropArea.model.selection

            onGeoSetterDropped: {
                var p = map.pixelsToGeo(pos.x, pos.y)
                var l = selection.items.length
                for (var i = 0; i < l; i++) {
                    var taggable = selection.items[i]
                    taggable.location = map.pixelsToGeo(pos.x, pos.y)
                }
            }
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
        }
    }

    Connections {
        target: dropArea.model
        onRowsInserted: map.lookAt(poiView.itemArea, 0, 80, 40)
    }
}

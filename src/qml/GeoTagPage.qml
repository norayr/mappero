import QtQuick 1.0
import Mappero 1.0
import "UIConstants.js" as UI

Item {
    anchors.fill: parent
    Keys.forwardTo: [ taggableView, toolBar, fileTools ]
    focus: true

    LayerManager {
        id: layerManager
    }

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

            mainLayer: layerManager.mainLayer
            center: Mappero.conf.lastPosition
            requestedZoomLevel: Mappero.conf.lastZoomLevel

            PathLayer {
                PathItem {
                    id: track
                    color: "red"
                }
            }

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
                    taggableView.setCurrent(index)
                    dropArea.model.selection.setSelection(index)
                }
            }
        }

        MapControls {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: UI.ToolSpacing
            map: map
            layerManager: layerManager
        }

        ToolBar {
            id: toolBar
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

            onTrackLoaded: {
                track.loadFile(filePath)
                map.lookAt(track.itemArea(), 0, 0, 40)
            }

            onHelp: helpLoader.source = "Help.qml"
        }

        Correlator {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: UI.ToolSpacing
            selection: dropArea.model.selection
            track: track
        }
    }

    Rectangle {
        id: tagPane
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: UI.TagPaneHeight
        color: "white"

        FileTools {
            id: fileTools
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: spacing

            model: dropArea.model
        }

        Rectangle {
            anchors.left: fileTools.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: fileTools.spacing
            color: "white"

            Text {
                anchors.fill: parent
                visible: dropArea.model.empty
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "#ddd"
                font.italic: true
                font.pixelSize: parent.height / 2
                text: qsTr("Drop your images here")
            }

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
    }

    Loader {
        id: helpLoader
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        focus: true
    }

    Connections {
        target: helpLoader.item
        onClose: helpLoader.source = ""
    }

    Connections {
        target: dropArea.model
        onRowsInserted: map.lookAt(poiView.itemArea, 0, 80, 40)
    }

    Connections {
        target: view
        onClosing: {
            Mappero.conf.lastPosition = map.center
            Mappero.conf.lastZoomLevel = map.zoomLevel
        }
    }
}

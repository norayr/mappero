import QtQml 2.2
import QtQuick 2.0
import Mappero 1.0
import Mardy 1.0
import "UIConstants.js" as UI

Item {
    anchors.fill: parent
    Keys.forwardTo: [ taggableView, toolBar, fileTools ]
    focus: true
    property bool __isPage: true

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
            center: Controller.conf.lastPosition
            requestedZoomLevel: Controller.conf.lastZoomLevel

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                enabled: Qt.platform.os != "osx"
                onWheel: {
                    map.zoomCenter = Qt.point(wheel.x, wheel.y)
                    map.requestedZoomLevel -= wheel.angleDelta.y / 120
                }
            }

            PinchArea {
                anchors.fill: parent
                onPinchStarted: {
                    map.zoomCenter = pinch.center
                    mapFlickable.interactive = false
                }
                onPinchUpdated: map.pinchScale = pinch.scale
                onPinchFinished: {
                    map.pinchScale = 0
                    mapFlickable.interactive = true
                }
            }

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
                    source: model.taggable.pixmapUrl
                    topText: model.fileName

                    onDragFinished: {
                        model.taggable.location = view.itemPos(model.index)
                    }

                    onPressed: view.setCurrent(model.index)
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
                map.lookAt(track.itemArea(), 0, 0, map.width / 4, map.height / 4)
            }

            onHelp: helpLoader.setSource("Help.qml", {
                    latestVersion: updater.latestVersion
                })
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

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                enabled: Qt.platform.os != "osx"
                onWheel: taggableView.flick(wheel.angleDelta.y * 4 - taggableView.horizontalVelocity, 0)
            }

            BusyMessage {
                id: busyMessage
                count: dropArea.model.busyTaggableCount
            }
        }
    }

    Loader {
        id: helpLoader
        anchors.fill: parent
        focus: true
    }

    Updater { id: updater
        versionUrl: "http://mappero.mardy.it/latestversion.json"
        property var timer: Timer {
            running: true
            interval: 2000
            onTriggered: updater.start()
        }
    }

    Connections {
        target: helpLoader.item
        onClose: helpLoader.source = ""
    }

    Connections {
        target: dropArea.model
        onRowsInserted: dropAreaInsertedTimer.restart()
        property var _t: Timer {
            id: dropAreaInsertedTimer
            interval: 100
            onTriggered: map.lookAt(poiView.itemArea, 0, 80,
                                    map.width / 5, map.height / 5)
        }
    }

    Connections {
        target: mainWindow
        onClosing: {
            console.log("got close request")
            busyMessage.gotCloseRequest = true;
            if (dropArea.model.busyTaggableCount > 0) {
                close.accepted = false
            } else {
                Controller.conf.lastPosition = map.center
                Controller.conf.lastZoomLevel = map.zoomLevel
            }
        }
    }

    Connections {
        target: application
        onItemsAddRequest: dropArea.model.addUrls(items)
    }
}

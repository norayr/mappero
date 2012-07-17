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

            property variant selectedIndexes: {}
            property variant selectedItems: itemsFromIndexes(selectedIndexes, model)
            property int lastIndex

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
                hasLocation: taggable.hasLocation
                dropItem: map

                onClicked: {
                    if (mouse.modifiers & Qt.ShiftModifier) {
                        ListView.view.setShiftSelection(index)
                    } else if (mouse.modifiers & Qt.ControlModifier) {
                        ListView.view.setCtrlSelection(index)
                    } else {
                        ListView.view.setSelection(index)
                    }
                    if (taggable.hasLocation) {
                        map.requestedCenter = taggable.location
                    }
                }
            }

            function setSelection(index) {
                var indexes = {}
                indexes[index] = true
                selectedIndexes = indexes
                lastIndex = index
            }

            function setShiftSelection(index) {
                if (lastIndex === undefined)
                    return setSelection(index)

                var indexes = selectedIndexes
                var first, last
                if (lastIndex > index) {
                    first = index
                    last = lastIndex
                } else {
                    first = lastIndex
                    last = index
                }
                for (var i = first; i <= last; i++) {
                    indexes[i] = true
                }
                selectedIndexes = indexes
                lastIndex = index
            }

            function setCtrlSelection(index) {
                var indexes = selectedIndexes
                if (indexes[index] === true) {
                    delete indexes[index]
                } else {
                    indexes[index] = true
                }
                selectedIndexes = indexes
                lastIndex = index
            }

            function itemsFromIndexes(indexes, model) {
                var selected = []
                for (var i in indexes) {
                    selected.push(model.get(i))
                }
                return selected;
            }
        }
    }

    Connections {
        target: dropArea.model
        onRowsInserted: map.lookAt(poiView.itemArea, 0, 80, 40)
    }
}

import QtQuick 1.0
import "UIConstants.js" as UI

Item {
    property int buttonSize: UI.OsmButtonSize
    property int screenMargin: UI.OsmScreenMargin
    property bool isPortrait: height > width
    property variant tracker

    Item {
        id: columns
        anchors.fill: parent
        anchors.leftMargin: screenMargin
        anchors.rightMargin: screenMargin
        property bool isPortrait: parent.isPortrait

        OsmButton {
            source: "qrc:osm-point"
        }

        OsmButton {
            source: "qrc:osm-path"
            Loader {
                id: trackPane
                Binding {
                    target: trackPane.item
                    property: "tracker"
                    value: tracker
                    when: trackPane.status == Loader.Ready
                }
            }
            onClicked: {
                trackPane.source = "OsmTrackItem.qml"
                trackPane.item.open()
            }
        }

        OsmButton {
            source: "qrc:osm-route"
        }

        OsmButton {
            source: "qrc:osm-go-to"
        }

        OsmButton {
            source: "qrc:osm-zoom-in"
            onClicked: map.requestedZoomLevel--
        }

        OsmButton {
            source: "qrc:osm-zoom-out"
            onClicked: map.requestedZoomLevel++
        }

        Item {
            width: buttonSize
            height: buttonSize
        }

        OsmButton {
            source: "qrc:osm-fullscreen"
            onClicked: view.switchFullscreen()
        }

        function layoutChildren() {
            if (isPortrait) {
                var increment = (height - screenMargin * 2 - buttonSize) / (children.length - 1)
                for (var i = 0; i < children.length; i++) {
                    var child = children[i]
                    child.anchors.right = undefined
                    child.anchors.left = columns.left
                    child.y = screenMargin + increment * i
                }
            } else {
                var increment = (height - screenMargin * 2 - buttonSize) / (children.length / 2 - 1)
                for (var i = 0; i < children.length / 2; i++) {
                    var child = children[i]
                    child.anchors.left = columns.left
                    child.y = screenMargin + increment * i
                }
                for (var i = children.length / 2; i < children.length; i++) {
                    var child = children[i]
                    child.anchors.left = undefined
                    child.anchors.right = columns.right
                    child.y = screenMargin + increment * (i - children.length / 2)
                }
            }
        }

        onHeightChanged: layoutChildren()
        onIsPortraitChanged: layoutChildren()
    }

    Row {
        id: bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: screenMargin
        anchors.leftMargin: screenMargin + buttonSize
        anchors.rightMargin: screenMargin + buttonSize
        height: buttonSize
        spacing: screenMargin

        OsmButton {
            source: gps.active ?
                "qrc:osm-gps-disable" :
                "qrc:osm-gps-enable"
            onClicked: {
                if (gps.active) gps.stop()
                else gps.start()
            }
        }

        OsmButton {
            source: "qrc:osm-settings"
        }
    }
}

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
            source: ":osm-point"
        }

        OsmButton {
            source: ":osm-path"
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
            source: ":osm-route"
        }

        OsmButton {
            source: ":osm-go-to"
        }

        Item {
            width: buttonSize
            height: buttonSize
        }

        Item {
            width: buttonSize
            height: buttonSize
        }

        Item {
            width: buttonSize
            height: buttonSize
        }

        Item {
            width: buttonSize
            height: buttonSize
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
                ":osm-gps-disable" :
                ":osm-gps-enable"
            onClicked: {
                if (gps.active) gps.stop()
                else gps.start()
            }
        }

        OsmButton {
            source: ":osm-settings"
        }
    }
}

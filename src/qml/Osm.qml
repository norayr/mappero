import QtQuick 1.0

Item {
    width: 800
    height: 480
    property int buttonSize: 72
    property int screenMargin: 8
    property bool isPortrait: height > width

    Item {
        id: columns
        anchors.fill: parent
        anchors.leftMargin: screenMargin
        anchors.rightMargin: screenMargin
        property bool isPortrait: parent.isPortrait

        OsmButton {
            source: "icon:maemo-mapper-point.png"
        }

        OsmButton {
            source: "icon:maemo-mapper-path.png"
        }

        OsmButton {
            source: "icon:maemo-mapper-route.png"
        }

        OsmButton {
            source: "icon:maemo-mapper-go-to.png"
        }

        OsmButton {
            source: "icon:maemo-mapper-zoom-in.png"
            onClicked: map.requestedZoomLevel--
        }

        OsmButton {
            source: "icon:maemo-mapper-zoom-out.png"
            onClicked: map.requestedZoomLevel++
        }

        Item {
            width: buttonSize
            height: buttonSize
        }

        OsmButton {
            source: "icon:maemo-mapper-fullscreen.png"
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
                "icon:maemo-mapper-gps-disable.png" :
                "icon:maemo-mapper-gps-enable.png"
            onClicked: {
                if (gps.active) gps.stop()
                else gps.start()
            }
        }

        OsmButton {
            source: "icon:maemo-mapper-settings.png"
        }
    }
}

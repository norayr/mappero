import QtQuick 2.0
import "UIConstants.js" as UI

Item {
    id: root
    property int buttonSize: UI.OsmButtonSize
    property int screenMargin: UI.OsmScreenMargin
    property bool isPortrait: height > width
    property variant tracker
    property variant searchBox
    property variant router

    states: [
        State {
            name: "allHidden"
            PropertyChanges { target: backButton; visible: false }
            PropertyChanges { target: searchButton; visible: false }
            PropertyChanges { target: pathButton; visible: false }
            PropertyChanges { target: routeButton; visible: false }
            PropertyChanges { target: goToButton; visible: false }
            PropertyChanges { target: fullScreenButton; visible: false }
            PropertyChanges { target: bottom; visible: false }
        },
        State {
            name: "searching"
            extend: "allHidden"
            PropertyChanges { target: searchButton; visible: true }
            PropertyChanges { target: searchBox; visible: true }
        },
        State {
            name: "routing"
            extend: "allHidden"
            when: router.isOpen && !router.browsing
        },
        State {
            name: "browsingRoutes"
            extend: "routing"
            when: router.browsing
            PropertyChanges {
                target: backButton;
                visible: true
                onClicked: router.stopBrowsing()
            }
        }
    ]

    OsmButton {
        id: backButton
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: screenMargin
        source: "qrc:osm-back"
        visible: false
    }

    Item {
        id: columns
        anchors.fill: parent
        anchors.leftMargin: screenMargin
        anchors.rightMargin: screenMargin
        property bool isPortrait: parent.isPortrait

        OsmButton {
            id: searchButton
            source: "qrc:osm-search"
            onClicked: {
                if (root.state == "searching") root.state = ""
                else root.state = "searching"
            }
        }

        OsmButton {
            id: pathButton
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
            id: routeButton
            source: "qrc:osm-route"
        }

        OsmButton {
            id: goToButton
            source: "qrc:osm-go-to"
        }

        OsmButton {
            id: zoomInButton
            source: "qrc:osm-zoom-in"
            onClicked: map.requestedZoomLevel--
        }

        OsmButton {
            id: zoomOutButton
            source: "qrc:osm-zoom-out"
            onClicked: map.requestedZoomLevel++
        }

        Item {
            width: buttonSize
            height: buttonSize
        }

        OsmButton {
            id: fullScreenButton
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

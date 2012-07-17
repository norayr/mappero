import QtQuick 1.1
import "UIConstants.js" as UI

Rectangle {
    id: root
    property alias source: thumbnail.source
    property alias bottomText: timeText.text
    property alias topText: nameText.text
    property alias hasLocation: locationMarker.visible
    property Item dropItem

    signal clicked(variant mouse)
    signal dropped(variant pos)

    color: "white"

    Image {
        id: thumbnail
        anchors.fill: parent
        anchors.margins: 2
        clip: true
        fillMode: Image.PreserveAspectCrop
        source: taggable.pixmapUrl
        sourceSize.width: UI.TaggableSourceWidth
        sourceSize.height: UI.TaggableSourceHeight

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 20
            Rectangle {
                anchors.fill: parent
                color: "#000"
                opacity: 0.6
            }
            Text {
                id: nameText
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#fff"
            }
        }

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 20
            Rectangle {
                anchors.fill: parent
                color: "#000"
                opacity: 0.6
            }
            Text {
                id: timeText
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#fff"
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: parent.clicked(mouse)
        hoverEnabled: true
    }

    Row {
        anchors.top: parent.top
        anchors.right: parent.right
        layoutDirection: Qt.RightToLeft
        spacing: 2

        Image {
            id: locationMarker
            width: UI.TaggableEmblemSize
            height: UI.TaggableEmblemSize
            source: ":taggable-has-pos.svg"
        }
    }

    Item {
        id: handleFrame
        anchors.top: parent.top
        anchors.left: parent.left
        width: UI.TaggableEmblemSize
        height: UI.TaggableEmblemSize
        visible: mouseArea.containsMouse

        Item {
            id: handle
            property real hotX: width / 2
            property real hotY: height

            width: parent.width
            height: parent.height
            Image {
                anchors.fill: parent
                source: ":taggable-handle.svg"
            }
            Image {
                id: cross
                x: handle.hotX - width / 2
                y: handle.hotY - height / 2
                visible: handleMouseArea.inDropItem
                source: ":taggable-handle-in.svg"
            }

            states: [
                State {
                    name: ""
                    PropertyChanges { target: handle; x: 0; y: 0 }
                },
                State {
                    name: "dragging"
                    when: handleMouseArea.drag.active
                    PropertyChanges { target: handle; opacity: 0.8 }
                }
            ]
        }

        MouseArea {
            id: handleMouseArea
            property bool inDropItem: isInDropItem()

            anchors.fill: parent
            drag.target: handle
            drag.onActiveChanged: {
                if (!drag.active) {
                    var coords = mapToItem(dropItem,
                                           handle.x + handle.hotX,
                                           handle.y + handle.hotY)
                    root.dropped(coords)
                }
            }

            function isInDropItem() {
                var coords = mapToItem(dropItem,
                                       handle.x + handle.hotX,
                                       handle.y + handle.hotY)
                return (coords.x >= 0 && coords.x <= dropItem.width &&
                        coords.y >= 0 && coords.y <= dropItem.height)
            }
        }
    }
}

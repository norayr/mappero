import QtQuick 1.1
import "UIConstants.js" as UI

Rectangle {
    id: root
    property alias source: thumbnail.source
    property alias bottomText: timeText.text
    property alias topText: nameText.text
    property bool selected: false

    signal clicked(variant mouse)

    color: selected ? "blue" : "white"

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
        hoverEnabled: true

        onClicked: parent.clicked(mouse)
        onDoubleClicked: taggable.open()
    }

    Row {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 2
        layoutDirection: Qt.RightToLeft
        spacing: 2

        Image {
            width: UI.TaggableEmblemSize
            height: UI.TaggableEmblemSize
            source: ":taggable-has-pos.svg"
            visible: taggable.hasLocation
        }
    }
}

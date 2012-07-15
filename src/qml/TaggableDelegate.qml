import QtQuick 1.0
import "UIConstants.js" as UI

Rectangle {
    property alias source: thumbnail.source
    property alias bottomText: timeText.text
    property alias topText: nameText.text
    property alias hasLocation: locationMarker.visible

    signal clicked

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

        Image {
            id: locationMarker
            anchors.right: parent.right
            anchors.top: parent.top
            width: 32
            height: 32
            source: ":taggable-has-pos.svg"
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: parent.clicked()
    }
}

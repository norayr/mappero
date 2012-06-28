import QtQuick 1.0
import "UIConstants.js" as UI

Rectangle {
    property variant location
    property alias source: thumbnail.source
    property alias topText: nameText.text

    color: "white"

    Image {
        id: thumbnail
        anchors.fill: parent
        anchors.margins: 1
        clip: true
        fillMode: Image.PreserveAspectCrop
        source: taggable.pixmapUrl
        sourceSize.width: UI.TaggableSourceWidth
        sourceSize.height: UI.TaggableSourceHeight

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 12
            Rectangle {
                anchors.fill: parent
                color: "#000"
                opacity: 0.6
            }
            Text {
                id: nameText
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#fff"
                font.pixelSize: 10
            }
        }
    }
}

import QtQuick 1.0

Rectangle {
    property alias source: thumbnail.source
    property alias bottomText: timeText.text
    property alias topText: nameText.text

    color: "white"

    Image {
        id: thumbnail
        anchors.fill: parent
        anchors.margins: 2
        clip: true
        fillMode: Image.PreserveAspectCrop
        source: taggable.pixmapUrl
        sourceSize.width: height
        sourceSize.height: height

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
}

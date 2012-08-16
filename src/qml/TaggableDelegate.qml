import QtQuick 1.1
import "UIConstants.js" as UI

Rectangle {
    id: root
    property alias source: thumbnail.source
    property alias bottomText: timeText.text
    property alias topText: nameText.text
    property bool selected: false

    signal clicked(variant mouse)

    color: "black"
    border.width: 0

    states: State {
        name: "selected"
        when: selected
        PropertyChanges {
            target: root
            border.color: "blue"; border.width: 2; y: 0
        }
    }

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
            height: 16
            Rectangle {
                anchors.fill: parent
                color: "#000"
                opacity: 0.6
            }
            Text {
                id: nameText
                anchors.fill: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "#fff"
                font.pixelSize: 10
            }
        }

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 16
            Rectangle {
                anchors.fill: parent
                color: "#000"
                opacity: 0.6
            }
            Text {
                id: timeText
                anchors.fill: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "#fff"
                font.pixelSize: 10
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
            source: ":taggable-has-pos"
            visible: taggable.hasLocation
        }

        Image {
            width: UI.TaggableEmblemSize
            height: UI.TaggableEmblemSize
            source: ":taggable-modified"
            visible: taggable.needsSave
        }
    }
}

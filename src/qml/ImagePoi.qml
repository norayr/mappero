import QtQuick 1.0
import Mappero 1.0
import "UIConstants.js" as UI

PoiItem {
    id: poiItem
    property alias source: thumbnail.source
    property alias topText: nameText.text
    transformOrigin: Item.Bottom

    signal dragFinished

    BorderImage {
        anchors.fill: parent
        border.bottom: 16
        border.left: 4
        border.right: 4
        border.top: 4

        source: ":poi-frame.svg"
        Image {
            id: thumbnail
            anchors.fill: parent
            anchors.topMargin: 4
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            anchors.bottomMargin: 16
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

    MouseArea {
        anchors.fill: parent
        drag.target: poiItem
        drag.filterChildren: true
        drag.onActiveChanged: if (!drag.active) poiItem.dragFinished()
    }
}

import QtQuick 2.0
import Mappero 1.0

PoiItem {
    id: root
    width: image.width
    height: image.height
    transformOrigin: Item.Bottom

    Image {
        id: image
        source: "qrc:/search-result.svg"
    }

    Rectangle {
        anchors.fill: label
        anchors.margins: -2
        color: "white"
        opacity: 0.8
        radius: 8
    }

    Text {
        id: label
        anchors.horizontalCenter: image.horizontalCenter
        anchors.top: image.bottom
        anchors.margins: 3
        width: Math.min(implicitWidth, 160)
        text: model.name
        elide: Text.ElideRight
    }
}

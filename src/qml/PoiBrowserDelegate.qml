import QtQuick 2.0
import Mappero 1.0

Item {
    id: root

    property string name
    property string address

    clip: true

    Column {
        anchors.fill: parent
        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            font.pixelSize: root.height / 2 * Math.min(1.0, root.width / contentWidth)
            text: name
        }
        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            font.pixelSize: root.height / 4 * Math.min(1.0, root.width / contentWidth)
            text: address
        }
    }
}

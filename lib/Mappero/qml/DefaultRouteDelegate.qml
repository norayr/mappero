import QtQuick 2.0
import Mappero 1.0

BaseRouteDelegate {
    id: root

    clip: true

    Column {
        anchors.fill: parent
        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            font.pixelSize: root.height / 3 * Math.min(1.0, root.width / contentWidth)
            text: qsTr("Length: %1").arg(Mappero.formatLength(pathItem.length))
        }
        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            font.pixelSize: root.height / 4 * Math.min(1.0, root.width / contentWidth)
            text: qsTr("Total time: %1").arg(Mappero.formatDuration(pathItem.endTime - pathItem.startTime))
        }
    }
}

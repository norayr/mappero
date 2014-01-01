import QtQuick 2.0
import Mappero 1.0

BaseWayPointDelegate {
    width: isNextWayPoint ? 32 : 12
    height: width
    transformOrigin: Item.Center

    Image {
        anchors.fill: parent
        source: "qrc:/direction/" + (model.data.dir ? model.data.dir : "unknown")
    }

    Text {
        anchors.top: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        text: model.text
        visible: isNextWayPoint
    }
}

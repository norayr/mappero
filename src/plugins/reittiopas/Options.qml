import QtQuick 2.0
import Mappero.Ui 1.0

Item {
    anchors.fill: parent
    implicitWidth: 300
    implicitHeight: 200

    Column {
        anchors.left: parent.left
        anchors.right: parent.right

        LabelLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            text: qsTr("Optimize for")

            SimpleSelector {
                width: 100
                model: ListModel {
                    id: listModel
                    ListElement { text: QT_TR_NOOP("default"); optimize: "default" }
                    ListElement { text: QT_TR_NOOP("speed"); optimize: "fastest" }
                    ListElement { text: QT_TR_NOOP("less walking"); optimize: "least_walking" }
                    ListElement { text: QT_TR_NOOP("less transfers"); optimize: "least_transfers" }
                }
                onSelectedIndexChanged: options["optimize"] = model.get(selectedIndex).optimize
            }
        }
        Rectangle {
            width: 50
            color: "blue"
            anchors.left: parent.left
            anchors.right: parent.right
        }
    }
}

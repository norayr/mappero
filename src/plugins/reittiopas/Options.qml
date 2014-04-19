import QtQuick 2.0
import Mappero.Ui 1.0

Item {
    property var options

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
                    ListElement { text: "speed" }
                    ListElement { text: "less walking" }
                    ListElement { text: "less transfers" }
                }
            }
        }
        Rectangle {
            width: 50
            color: "blue"
            anchors.left: parent.left
            anchors.right: parent.right
        }
    }

    Text {
        anchors.fill: parent
        text: "Here are the options"
    }
}

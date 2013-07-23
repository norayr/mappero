import QtQuick 2.0

Item {
    id: root

    property real value: 0

    height: 17

    Text {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        text: qsTr("Time zone offset:")
    }

    Row {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right

        Image {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: height
            source: "qrc:tz-minus"

            MouseArea {
                anchors.fill: parent
                onClicked: if (root.value > -24) root.value--
            }
        }

        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: height * 1.5
            color: "#eee"

            Text {
                id: textValue
                anchors.fill: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: value
            }
        }

        Image {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: height
            source: "qrc:tz-plus"

            MouseArea {
                anchors.fill: parent
                onClicked: if (root.value < 24) root.value++
            }
        }
    }
}

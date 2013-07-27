import QtQuick 1.0
import Mappero 1.0

Item {
    id: slider

    property real value: 0
    property real maximum: 1800
    property real minimum: -1800
    property int xMax: width - handle.width

    height: 17

    Ticks {
        anchors.fill: parent
        anchors.leftMargin: handle.width / 2
        anchors.rightMargin: handle.width / 2
    }

    Image {
        id: handle
        x: slider.xMax / 2
        y: 2;
        width: height; height: slider.height - 4
        source: "qrc:time-slider-handle"

        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.top
            width: 50
            height: 18
            visible: mouse.drag.active

            Rectangle {
                anchors.fill: parent
                color: "white"
                radius: 8
            }

            Text {
                id: tip
                anchors.horizontalCenter: parent.horizontalCenter
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: Mappero.formatOffset(slider.value)
            }
        }

        MouseArea {
            id: mouse
            anchors.centerIn: parent
            width: 32; height: 32
            drag {
                target: parent
                axis: Drag.XAxis
                minimumX: 0; maximumX: slider.xMax
            }

            onPositionChanged: {
                value = (maximum - minimum) * handle.x / slider.xMax + minimum;
            }
        }
    }
}

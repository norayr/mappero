import QtQuick 2.0

Rectangle {
    id: root

    property var hotSpot: null
    property bool hovered: false
    property alias text: label.text

    color: "white"
    radius: 3
    border.width: 1
    x: hotSpot ? (hotSpot.x - width / 2) : 0
    y: hotSpot ? (hotSpot.y - height / 2) : 0
    width: label.width + 10
    height: label.height + 6
    visible: hovered

    Text {
        id: label
        anchors.centerIn: parent
    }
}

import QtQuick 1.0

Item {
    id: root

    property url source

    signal clicked

    Image {
        anchors.fill: parent
        source: enabled ? root.source : root.source + "-off"
    }

    MouseArea {
        anchors.fill: parent
        onClicked: parent.clicked()
    }
}

import QtQuick 2.0

Item {
    id: root

    property url source
    property bool useAlternateWhenDisabled: true

    signal clicked

    Image {
        anchors.fill: parent
        source: (enabled || !useAlternateWhenDisabled) ? root.source : root.source + "-off"
    }

    Rectangle {
        anchors.fill: parent
        color: "white"
        opacity: 0.5
        visible: (!enabled && !useAlternateWhenDisabled)
    }

    MouseArea {
        anchors.fill: parent
        onClicked: parent.clicked()
    }
}

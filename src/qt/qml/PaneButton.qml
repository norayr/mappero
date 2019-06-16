import QtQuick 2.0
import "UIConstants.js" as UI

Rectangle {
    id: root

    property alias text: label.text

    signal clicked

    anchors.left: parent.left
    anchors.right: parent.right
    height: UI.PaneButtonHeight
    color: enabled ? "#dfdfd0" : "#eee"
    radius: UI.PaneButtonHeight / 2

    Text {
        id: label
        anchors.centerIn: parent
        color: root.enabled ? "#000" : "#888"
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.clicked()
    }
}

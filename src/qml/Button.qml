import QtQuick 2.0
import "UIConstants.js" as UI

Rectangle {
    id: root

    property alias text: textField.text
    property bool selected: false

    signal clicked()

    anchors {
        left: parent.left
        right: parent.right
    }
    height: UI.PaneButtonHeight
    radius: height / 3
    color: selected ? "#dfdfd0" : "#eee"

    Text {
        id: textField
        anchors.centerIn: parent
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.clicked()
    }
}

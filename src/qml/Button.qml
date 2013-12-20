import QtQuick 2.0
import "UIConstants.js" as UI

Rectangle {
    id: root

    property alias text: textField.text
    property bool selected: false
    property alias leftItems: leftRow.children
    property alias rightItems: rightRow.children

    signal clicked()

    anchors {
        left: parent.left
        right: parent.right
    }
    height: UI.PaneButtonHeight
    radius: height / 3
    color: selected ? "#dfdfd0" : "#eee"

    MouseArea {
        anchors.fill: parent
        onClicked: root.clicked()
    }

    Row {
        id: leftRow
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
    }

    Item {
        anchors {
            left: leftRow.right
            right: rightRow.left
            top: parent.top
            bottom: parent.bottom
        }

        Text {
            id: textField
            anchors.centerIn: parent
        }
    }

    Row {
        id: rightRow
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
    }
}

import QtQuick 2.0
import "UIConstants.js" as UI

FocusScope {
    id: root

    property alias text: textField.text

    anchors.left: parent.left
    anchors.right: parent.right
    height: UI.PaneButtonHeight

    Rectangle {
        id: background

        anchors.fill: parent
        radius: height / 3
        color: textField.activeFocus ? "#dfdfd0" : "#eee"

        TextInput {
            id: textField

            anchors {
                left: parent.left
                right: parent.right
                margins: background.radius / 2
                verticalCenter: parent.verticalCenter
            }
            clip: true
            visible: activeFocus
        }

        Text {
            id: staticText

            anchors {
                left: parent.left
                right: parent.right
                margins: background.radius / 2
                verticalCenter: parent.verticalCenter
            }
            text: textField.text
            elide: Text.ElideRight
            visible: !textField.visible

            MouseArea {
                anchors.fill: parent
                onClicked: textField.forceActiveFocus()
            }
        }
    }
}

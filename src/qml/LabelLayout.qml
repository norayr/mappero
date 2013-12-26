import QtQuick 2.0
import "UIConstants.js" as UI

Item {
    id: root

    property alias text: label.text
    default property alias children: widget.children

    anchors.left: parent.left
    anchors.right: parent.right
    height: childrenRect.height

    states: [
        State {
            name: "vertical"
            AnchorChanges {
                target: label
                anchors.top: parent.top
                anchors.verticalCenter: undefined
            }
            AnchorChanges {
                target: widget
                anchors.left: parent.left
                anchors.top: label.bottom
            }
            PropertyChanges {
                target: widget
                anchors.leftMargin: 0
            }
        }
    ]

    transitions: [
        Transition { AnchorAnimation {}}
    ]

    Text {
        id: label
        anchors.left: parent.left
        anchors.verticalCenter: widget.verticalCenter
        width: contentWidth
    }

    Item {
        id: widget
        anchors.top: parent.top
        anchors.left: label.right
        anchors.leftMargin: UI.FieldHSpacing
        anchors.right: parent.right
        height: childrenRect.height
    }
}

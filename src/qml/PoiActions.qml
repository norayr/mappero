import QtQuick 2.0
import Mappero 1.0

Popup {
    id: root

    signal destinationSet()
    signal originSet()

    Flickable {
        id: view
        width: 200
        height: Math.min(contentHeight, 300)
        contentHeight: contentItem.childrenRect.height

        Column {
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }

            spacing: 2

            Button {
                text: qsTr("Set as destination")
                onClicked: { root.close(); root.destinationSet() }
            }

            Button {
                text: qsTr("Set as starting point")
                onClicked: { root.close(); root.originSet() }
            }
        }
    }
}

import QtQuick 2.0
import Mappero.Ui 1.0

Popup {
    id: root

    signal destinationSet()
    signal originSet()

    minimumWidth: 200
    minimumHeight: view.contentHeight

    Flickable {
        id: view
        anchors.fill: parent
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

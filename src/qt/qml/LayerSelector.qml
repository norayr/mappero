import Mappero.Ui 1.0
import QtQuick 2.0
import "UIConstants.js" as UI

Popup {
    id: root
    property var manager

    minimumWidth: 200
    minimumHeight: 250

    ListView {
        id: view
        anchors.fill: parent
        clip: true
        spacing: 2
        model: manager.model
        currentIndex: manager.mainLayerIndex
        onCurrentIndexChanged: manager.setLayer(currentIndex)
        delegate: Rectangle {
            width: view.width
            height: UI.PaneButtonHeight
            radius: height / 3
            color: ListView.isCurrentItem ? "#dfdfd0" : "#eee"
            Text {
                anchors.centerIn: parent
                text: model.name
            }
            MouseArea {
                anchors.fill: parent
                onClicked: { view.currentIndex = index; root.close() }
            }
        }
    }
}

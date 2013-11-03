import QtQuick 2.0
import "UIConstants.js" as UI

Popup {
    id: root
    property int selectedIndex: 0
    property alias model: view.model

    ListView {
        id: view
        width: 200
        height: (UI.PaneButtonHeight + spacing) * count
        clip: true
        spacing: 2
        currentIndex: selectedIndex
        onCurrentIndexChanged: selectedIndex = currentIndex
        delegate: Rectangle {
            width: view.width
            height: UI.PaneButtonHeight
            radius: height / 3
            color: ListView.isCurrentItem ? "#dfdfd0" : "#eee"
            Text {
                anchors.centerIn: parent
                text: model.displayName
            }
            MouseArea {
                anchors.fill: parent
                onClicked: { view.currentIndex = index; root.close() }
            }
        }
    }
}

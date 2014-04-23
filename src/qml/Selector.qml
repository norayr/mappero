import Mappero 1.0
import QtQuick 2.0

Item {
    id: root

    property alias buttonDelegate: button.delegate
    property Component listDelegate: buttonDelegate
    property var model
    property int selectedIndex: 0
    width: button.contentWidth
    height: button.currentItem ? button.currentItem.height : 10

    ListView {
        id: button
        anchors.fill: parent
        clip: true
        interactive: false
        model: root.model
        currentIndex: root.selectedIndex
        spacing: 2
        highlightMoveDuration: 1

        Connections {
            target: button.currentItem
            ignoreUnknownSignals: true
            onClicked: {
                if (popup.isOpen) {
                    popup.close()
                } else {
                    popup.open()
                }
            }
        }
    }

    Popup {
        id: popup

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.bottom
        source: root
        minimumWidth: 200
        minimumHeight: view.contentItem.childrenRect.height

        SelectorView {
            id: view
            model: root.model
            currentIndex: root.selectedIndex
            delegate: root.listDelegate
            onItemSelected: { root.selectedIndex = currentIndex; popup.close() }
        }
    }
}

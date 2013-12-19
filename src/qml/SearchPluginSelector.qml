import QtQuick 2.0

Popup {
    id: root
    property int selectedIndex: 0
    property alias model: view.model

    ListView {
        id: view
        width: 200
        height: contentItem.childrenRect.height
        clip: true
        spacing: 2
        currentIndex: selectedIndex
        onCurrentIndexChanged: selectedIndex = currentIndex
        delegate: Button {
            id: listItem
            width: view.width
            text: model.displayName
            selected: ListView.isCurrentItem
            onClicked: { view.currentIndex = index; root.close() }
        }
    }
}

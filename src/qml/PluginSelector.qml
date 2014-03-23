import QtQuick 2.0

Popup {
    id: root
    property alias selectedIndex: view.currentIndex
    property alias model: view.model

    minimumWidth: 200
    minimumHeight: view.contentItem.childrenRect.height

    SelectorView {
        id: view
        delegate: Button {
            text: model.displayName
            selected: ListView.isCurrentItem
            spacing: 2
            rightItems: [
                Image {
                    height: parent.height
                    width: height
                    source: model.icon
                }
            ]
            onClicked: view.selectItem(index)
        }

        onItemSelected: root.close()
    }
}

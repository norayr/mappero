import QtQuick 2.0

Popup {
    id: root
    property int selectedIndex: 0
    property alias model: view.model

    minimumWidth: 200
    minimumHeight: view.contentItem.childrenRect.height

    ListView {
        id: view
        anchors.fill: parent
        clip: true
        spacing: 2
        currentIndex: selectedIndex
        onCurrentIndexChanged: selectedIndex = currentIndex
        delegate: Button {
            id: listItem
            width: view.width
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
            onClicked: { view.currentIndex = index; root.close() }
        }
    }
}

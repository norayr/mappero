import QtQuick 2.0

ListView {
    id: root

    signal itemSelected

    anchors.fill: parent
    clip: true
    spacing: 2

    function selectItem(index) {
        currentIndex = index
        itemSelected()
    }
}

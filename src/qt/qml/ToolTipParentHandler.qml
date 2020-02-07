import QtQuick 2.0

MouseArea {
    id: root

    property var activeItem: null

    hoverEnabled: true
    acceptedButtons: Qt.NoButton

    onMouseXChanged: checkToolTip()
    onMouseYChanged: checkToolTip()
    onEntered: checkToolTip()
    onExited: activeItem = null

    function getToolTipTarget() {
        var item = row.childAt(mouseX, mouseY)
        if (!item || !item.enabled) return null
        if (!item.hasOwnProperty("toolTip")) return null
        return item
    }

    function checkToolTip() {
        root.activeItem = getToolTipTarget()
    }
}

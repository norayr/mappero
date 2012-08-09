import QtQuick 1.0
import "UIConstants.js" as UI

ListView {
    property variant selectedItems:  model.selection.items

    orientation: ListView.Horizontal
    cacheBuffer: 40
    clip: true
    highlightMoveDuration: 500
    spacing: 2
    delegate: TaggableDelegate {
        id: delegate
        height: ListView.view.height - 6
        y: 6
        width: height
        source: taggable.pixmapUrl
        topText: model.fileName
        bottomText: Qt.formatDateTime(model.time, "d/M/yyyy hh:mm")
        selected: ListView.view.model.selection.isSelected(index)

        onClicked: ListView.view.select(index, mouse.modifiers)

        ListView.onRemove: SequentialAnimation {
            PropertyAction { target: delegate; property: "ListView.delayRemove"; value: true }
            NumberAnimation { target: delegate; properties: "scale,width"; to: 0; duration: 250; easing.type: Easing.InOutQuad }
            PropertyAction { target: delegate; property: "ListView.delayRemove"; value: false }
        }

        Connections {
            target: delegate.ListView.view.model.selection
            onItemsChanged: delegate.selected = delegate.ListView.view.model.selection.isSelected(index)
        }
    }

    SequentialAnimation {
        id: highlightAnimation
        running: false
        alwaysRunToEnd: true
        NumberAnimation { target: currentItem; properties: "scale"; to: 0.8; duration: 100; easing.type: Easing.InCubic; easing.amplitude: 1 }
        NumberAnimation { target: currentItem; properties: "scale"; to: 1; duration: 400; easing.type: Easing.OutElastic; easing.amplitude: 1 }
    }

    function setCurrent(index) {
        if (index == currentIndex) currentIndex = -1
        currentIndex = index
        highlightAnimation.running = true
    }

    function select(index, modifiers) {
        if (index < 0 || index >= model.rowCount()) return
        if (modifiers & Qt.ShiftModifier) {
            model.selection.setShiftSelection(index)
        } else if (modifiers & Qt.ControlModifier) {
            model.selection.setCtrlSelection(index)
        } else {
            model.selection.setSelection(index)
        }
        currentIndex = index
    }

    Keys.onPressed: {
        if (event.key == Qt.Key_N) {
            select(model.selection.nextUntagged(), event.modifiers)
        } else if (event.key == Qt.Key_Left) {
            select(currentIndex - 1, event.modifiers)
        } else if (event.key == Qt.Key_Right) {
            select(currentIndex + 1, event.modifiers)
        } else {
            return
        }
        event.accepted = true
    }
}

import QtQuick 1.0
import "UIConstants.js" as UI

ListView {
    property variant selectedItems:  model.selection.items

    orientation: ListView.Horizontal
    cacheBuffer: 40
    clip: true
    highlightMoveDuration: 500
    highlightRangeMode: ListView.StrictlyEnforceRange
    preferredHighlightBegin: 0
    preferredHighlightEnd: width
    spacing: 2
    delegate: TaggableDelegate {
        id: delegate
        height: ListView.view.height
        width: height
        source: taggable.pixmapUrl
        topText: model.fileName
        bottomText: Qt.formatDateTime(model.time, "d/M/yyyy hh:mm")
        selected: ListView.view.model.selection.isSelected(index)

        onClicked: {
            if (mouse.modifiers & Qt.ShiftModifier) {
                ListView.view.model.selection.setShiftSelection(index)
            } else if (mouse.modifiers & Qt.ControlModifier) {
                ListView.view.model.selection.setCtrlSelection(index)
            } else {
                ListView.view.model.selection.setSelection(index)
            }
            ListView.view.currentIndex = index
        }

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
}

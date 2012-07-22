import QtQuick 1.0
import "UIConstants.js" as UI

ListView {
    property variant selectedItems:  model.selection.items

    orientation: ListView.Horizontal
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
        }

        Connections {
            target: delegate.ListView.view.model.selection
            onItemsChanged: delegate.selected = delegate.ListView.view.model.selection.isSelected(index)
        }
    }
}

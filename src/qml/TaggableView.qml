import QtQuick 1.0
import "UIConstants.js" as UI

ListView {
    property variant selectedIndexes: {}
    property variant selectedItems: itemsFromIndexes(selectedIndexes, model)
    property int lastIndex

    orientation: ListView.Horizontal
    spacing: 2
    delegate: TaggableDelegate {
        height: ListView.view.height
        width: height
        source: taggable.pixmapUrl
        topText: model.fileName
        bottomText: Qt.formatDateTime(model.time, "d/M/yyyy hh:mm")
        selected: ListView.view.isSelected(index)

        onClicked: {
            if (mouse.modifiers & Qt.ShiftModifier) {
                ListView.view.setShiftSelection(index)
            } else if (mouse.modifiers & Qt.ControlModifier) {
                ListView.view.setCtrlSelection(index)
            } else {
                ListView.view.setSelection(index)
            }
        }
    }

    function setSelection(index) {
        var indexes = {}
        indexes[index] = true
        selectedIndexes = indexes
        lastIndex = index
    }

    function setShiftSelection(index) {
        if (lastIndex === undefined)
            return setSelection(index)

        var indexes = selectedIndexes
        var first, last
        if (lastIndex > index) {
            first = index
            last = lastIndex
        } else {
            first = lastIndex
            last = index
        }
        for (var i = first; i <= last; i++) {
            indexes[i] = true
        }
        selectedIndexes = indexes
        lastIndex = index
    }

    function setCtrlSelection(index) {
        var indexes = selectedIndexes
        if (indexes[index] === true) {
            delete indexes[index]
        } else {
            indexes[index] = true
        }
        selectedIndexes = indexes
        lastIndex = index
    }

    function isSelected(index) {
        return selectedIndexes[index] === true
    }

    function itemsFromIndexes(indexes, model) {
        var selected = []
        for (var i in indexes) {
            selected.push(model.get(i))
        }
        return selected;
    }
}

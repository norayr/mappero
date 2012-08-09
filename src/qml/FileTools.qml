import QtQuick 1.0
import "UIConstants.js" as UI

Column {
    id: root

    property variant model
    property variant selection: model.selection

    width: UI.FileToolsWidth
    spacing: 2

    PaneButton {
        text: qsTr("Select all")
        onClicked: selection.selectAll()
        enabled: !model.empty
    }

    PaneButton {
        text: qsTr("Save")
        enabled: selection.needsSave
        onClicked: root.save()
    }

    PaneButton {
        text: qsTr("Clear")
        onClicked: selection.removeItems()
        enabled: !selection.empty
    }

    Keys.onPressed: {
        if (event.key == Qt.Key_C) {
            selection.removeItems()
        } else if (event.key == Qt.Key_S) {
            save()
        } else if (event.key == Qt.Key_X) {
            save()
            selection.removeItems()
        } else if (event.key == Qt.Key_A &&
                   event.modifiers & Qt.ControlModifier) {
            selection.selectAll()
        } else {
            return
        }
        event.accepted = true
    }

    function save() {
        var l = selection.items.length
        for (var i = 0; i < l; i++) {
            var taggable = selection.items[i]
            taggable.save()
        }
    }
}

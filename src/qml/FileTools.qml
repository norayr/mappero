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
        text: qsTr("Clear")
        onClicked: selection.removeItems()
        enabled: !selection.empty
    }
}

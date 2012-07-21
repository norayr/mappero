import QtQuick 1.0
import "UIConstants.js" as UI

Row {
    id: root

    property variant selectedItems: []

    signal geoSetterDropped(variant pos)

    spacing: UI.ToolSpacing

    GeoSetter {
        id: geoSetter
        width: UI.TaggableToolsSize
        height: UI.TaggableToolsSize
        selectedItems: root.selectedItems

        onDropped: root.geoSetterDropped(pos)
    }

    UndoButton {
        width: UI.TaggableToolsSize
        height: UI.TaggableToolsSize
        selectedItems: root.selectedItems
    }

    TagRemoveButton {
        width: UI.TaggableToolsSize
        height: UI.TaggableToolsSize
        selectedItems: root.selectedItems
    }
}

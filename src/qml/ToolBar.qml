import QtQuick 1.0
import "UIConstants.js" as UI

Row {
    id: root

    property variant selection

    signal geoSetterDropped(variant pos)

    spacing: UI.ToolSpacing

    GeoSetter {
        id: geoSetter
        width: UI.TaggableToolsSize
        height: UI.TaggableToolsSize
        selectedItems: root.selection.items

        onDropped: root.geoSetterDropped(pos)
    }

    ItemAction {
        width: UI.TaggableToolsSize
        height: UI.TaggableToolsSize
        selectedItems: root.selection.items
        enabled: root.selection.needsSave

        source: ":taggable-reload"
        onActivate: taggable.reload()
    }

    ItemAction {
        width: UI.TaggableToolsSize
        height: UI.TaggableToolsSize
        selectedItems: root.selection.items
        enabled: root.selection.hasLocation

        source: ":tag-remove"
        onActivate: taggable.clearLocation()
    }

    ItemAction {
        width: UI.TaggableToolsSize
        height: UI.TaggableToolsSize
        selectedItems: root.selection.items
        enabled: root.selection.needsSave

        source: ":taggable-save"
        onActivate: taggable.save()
    }
}

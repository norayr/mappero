import QtQuick 1.0
import "UIConstants.js" as UI

Row {
    id: root

    property variant selectedItems: []

    signal geoSetterDropped(variant pos)

    spacing: UI.ToolSpacing

    GeoSetter {
        id: geoSetter
        selectedItems: root.selectedItems

        onDropped: root.geoSetterDropped(pos)
    }
}

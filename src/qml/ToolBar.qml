import QtQuick 1.0
import "UIConstants.js" as UI

Row {
    id: root

    property variant selection

    signal geoSetterDropped(variant pos)
    signal trackLoaded(string filePath)
    signal help()

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

    ImageButton {
        width: UI.TaggableToolsSize
        height: UI.TaggableToolsSize

        source: ":osm-path" // FIXME
        onClicked: fileChooserLoad.open()

        FileDialog {
            id: fileChooserLoad
            visible: true
            title: "Choose a file"
            folder: "."
            selectExisting: true
            selectMultiple: false
            nameFilters: [ "GPS tracks (*.gpx *.kml)", "All files (*.*)" ]

            onAccepted: root.trackLoaded(filePath)
        }
    }

    ImageButton {
        width: UI.TaggableToolsSize
        height: UI.TaggableToolsSize

        source: ":help"
        onClicked: root.help()
    }

    Keys.onPressed: {
        if (event.key == Qt.Key_R) {
            var l = selection.items.length
            for (var i = 0; i < l; i++) {
                var taggable = selection.items[i]
                taggable.reload()
            }
        } else if (event.key == Qt.Key_D) {
            var l = selection.items.length
            for (var i = 0; i < l; i++) {
                var taggable = selection.items[i]
                taggable.clearLocation()
            }
        } else if (event.key == Qt.Key_H ||
                   event.key == Qt.Key_Help ||
                   event.key == Qt.Key_F1 ||
                   event.key == Qt.Key_Question) {
            help()
        } else {
            return
        }
        event.accepted = true
    }
}

import Mappero.Ui 1.0
import QtQuick 2.0
import Qt.labs.platform 1.0
import "UIConstants.js" as UI

Item {
    id: root

    property variant selection

    signal geoSetterDropped(variant pos)
    signal trackLoaded(url filePath)
    signal help()

    width: row.width + 2 * UI.ToolbarMargins
    height: row.height + 2 * UI.ToolbarMargins

    PaneBackground {}

    Row {
        id: row
        anchors.centerIn: parent
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

            source: "qrc:taggable-reload"
            onActivate: taggable.reload()
        }

        ItemAction {
            width: UI.TaggableToolsSize
            height: UI.TaggableToolsSize
            selectedItems: root.selection.items
            enabled: root.selection.hasLocation

            source: "qrc:tag-remove"
            onActivate: taggable.clearLocation()
        }

        ImageButton {
            width: UI.TaggableToolsSize
            height: UI.TaggableToolsSize

            source: "qrc:correlate"
            onClicked: fileChooserLoad.open()

            FileDialog {
                id: fileChooserLoad
                title: "Choose a file"
                nameFilters: [ "GPS tracks (*.gpx *.kml)", "All files (*.*)" ]

                onAccepted: root.trackLoaded(file)
            }
        }

        ImageButton {
            width: UI.TaggableToolsSize
            height: UI.TaggableToolsSize

            source: "qrc:help"
            onClicked: root.help()
        }
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

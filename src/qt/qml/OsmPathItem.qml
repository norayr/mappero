import Mappero.Ui 1.0
import QtQuick 2.0
import Qt.labs.platform 1.0

Pane {
    id: pane

    property var path

    Column {
        width: 400

        MenuButton {
            text: "Open..."
            onClicked: {
                fileChooserLoad.open()
            }
        }
        MenuButton {
            text: "Save..."
            enabled: !path.empty
            onClicked: {
                fileChooserSave.open()
            }
        }
        MenuButton {
            text: "Clear path"
            enabled: !path.empty
            onClicked: { path.clear(); pane.close() }
        }
    }

    resources: [
        FileDialog {
            id: fileChooserSave
            title: "Choose a file name"
            fileMode: FileDialog.SaveFile
            //fileUrl: Qt.formatDate(new Date(), Qt.ISODate) + ".gpx"

            onFolderChanged: { console.log("Folder changed: " + folder) }
            onAccepted: {
                pane.close()
                console.log("Dialog accepted: " + file)
                path.saveFile(file);
            }
        },

        FileDialog {
            id: fileChooserLoad
            title: "Choose a file"
            fileMode: FileDialog.OpenFile
            nameFilters: [ "Tracks (*.gpx *.kml)" ]

            onFolderChanged: { console.log("Folder changed: " + folder) }
            onAccepted: {
                pane.close()
                console.log("Dialog accepted: " + file)
                path.loadFile(file);
            }
        }
    ]
}

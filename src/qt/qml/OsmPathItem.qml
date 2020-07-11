import Mappero.Ui 1.0
import QtQuick 2.0
import QtQuick.Dialogs 1.0

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
            folder: "/home/user"
            selectExisting: false
            selectMultiple: false
            //fileUrl: Qt.formatDate(new Date(), Qt.ISODate) + ".gpx"

            onFolderChanged: { console.log("Folder changed: " + folder) }
            onAccepted: {
                pane.close()
                console.log("Dialog accepted: " + fileUrl)
                path.saveFile(fileUrl);
            }
        },

        FileDialog {
            id: fileChooserLoad
            title: "Choose a file"
            folder: "."
            selectExisting: true
            selectMultiple: false
            nameFilters: [ "Tracks (*.gpx)", "Packages (*.deb)" ]

            onFolderChanged: { console.log("Folder changed: " + folder) }
            onAccepted: {
                pane.close()
                console.log("Dialog accepted: " + fileUrl)
                path.loadFile(fileUrl);
            }
        }
    ]
}

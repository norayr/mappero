import QtQuick 2.0
import QtQuick.Dialogs 1.0

Pane {
    id: pane

    property variant tracker

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
            onClicked: {
                fileChooserSave.open()
            }
        }
        MenuButton {
            text: "Clear path"
            onClicked: { tracker.clear(); pane.close() }
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
                tracker.saveFile(fileUrl);
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
                tracker.loadFile(fileUrl);
            }
        }
    ]
}

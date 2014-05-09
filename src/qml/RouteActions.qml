import QtQuick 2.0
import QtQuick.Dialogs 1.0
import Mappero.Ui 1.0

Popup {
    id: root
    property var path

    signal activated()

    Flickable {
        id: view
        width: 200
        height: Math.min(contentHeight, 300)
        contentHeight: contentItem.childrenRect.height

        Column {
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }

            spacing: 2

            Button {
                text: qsTr("Set as route")
                onClicked: { root.close(); root.activated() }
            }

            Button {
                text: qsTr("Save")
                onClicked: fileChooserSave.open()
            }
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
                root.close();
                console.log("Dialog accepted: " + fileUrl)
                path.saveFile(fileUrl);
            }
        }
    ]
}

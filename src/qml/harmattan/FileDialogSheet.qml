import QtQuick 1.0
import com.nokia.meego 1.0

Sheet {
    id: root

    property string titleText: ""
    property bool selectFolder: false
    property bool selectExisting: true
    property alias folder: fileBrowser.folder
    property alias nameFilters: fileBrowser.nameFilters
    property alias selectMultiple: fileBrowser.selectMultiple
    property string filePath: selectExisting ? fileBrowser.filePath :
                                fileBrowser.folder + "/" + fileName
    property alias fileName: nameBox.text
    //property alias filePaths: fileBrowser.filePaths

    acceptButtonText: qsTr("Save")
    rejectButtonText: qsTr("Cancel")

    content: Item {
        anchors.fill: parent

        FileBrowser {
            id: fileBrowser
            selectFolder: root.selectFolder
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: nameBox.visible ? nameBox.top : parent.bottom
            platformStyle: FileBrowserStyle {}
        }

        TextField {
            id: nameBox
            visible: !root.selectExisting
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            placeholderText: qsTr("Enter a file name")
        }
    }
}

import QtQuick 1.0
import com.nokia.meego 1.0

/* We really want to inherit from CommonDialog, but MeeGo doesn't export it */
SelectionDialog {
    id: root

    property bool selectFolder: false
    property bool selectExisting: true
    property alias folder: fileBrowser.folder
    property alias nameFilters: fileBrowser.nameFilters
    property alias selectMultiple: fileBrowser.selectMultiple
    property alias filePath: fileBrowser.filePath
    property string fileName: ""
    //property alias filePaths: fileBrowser.filePaths

    height: screen.displayHeight
    width: screen.displayWidth - (platformStyle.leftMargin +
                                  platformStyle.rightMargin)

    content: Item {
        height: root.height - 100
        width: parent.width

        FileBrowser {
            id: fileBrowser
            selectFolder: root.selectFolder
            anchors.fill: parent
            platformStyle: FileBrowserStyle { inverted: true }

            onFilePathChanged: accept()
        }
    }
}

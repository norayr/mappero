import QtQuick 1.0
import com.nokia.meego 1.0

Item {
    id: root

    property string title: ""
    property Item visualParent
    property bool selectFolder: false
    property bool selectExisting: true
    property string folder
    property variant nameFilters: ["*"]
    property bool selectMultiple
    property string fileName: ""
    property string filePath: loader.item ? loader.item.filePath : ""
    property variant filePaths: loader.item ? loader.item.filePaths : []

    signal accepted

    Loader {
        id: loader

        onLoaded: {
            item.titleText = title
            item.selectFolder = selectFolder
            item.selectExisting = selectExisting
            item.folder = folder
            item.nameFilters = nameFilters
            item.selectMultiple = selectMultiple
            item.fileName = fileName
            item.visualParent = visualParent
        }
    }

    Connections {
        target: loader.item
        onAccepted: {
            fileName = loader.item.fileName
            filePath = loader.item.filePath
            folder = loader.item.folder

            accepted()
        }
    }

    function open() {
        loader.source = (selectExisting && !selectMultiple) ?
            "FileDialogDialog.qml" : "FileDialogSheet.qml"
        loader.item.open()
    }
}

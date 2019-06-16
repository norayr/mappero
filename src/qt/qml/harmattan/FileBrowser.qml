import QtQuick 1.0
import com.nokia.meego 1.0
import Qt.labs.folderlistmodel 1.0

Item {
    id: root

    property bool selectFolder: false
    property bool selectMultiple: false
    property string filePath: ""
    property string folder
    property variant nameFilters

    property Style platformStyle: FileBrowserStyle {}

    property string __iconStyle: platformStyle.inverted ? "-white" : ""
    property bool __folderSet: false
    property bool __folderChanged: false

    onFolderChanged: {
        if (__folderChanged) {
            __folderChanged = false;
        } else {
            __folderSet = true
            folderModel.folder = local2url(folder)
        }
    }

    Item {
        id: toolBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: backButton.height
        Button {
            id: backButton
            anchors.left: parent.left
            width: 160
            platformStyle: ButtonStyle {inverted: root.platformStyle.inverted }
            text: qsTr("Up")
            iconSource: "image://theme/icon-m-toolbar-previous" + __iconStyle
            onClicked: folderModel.folder = folderModel.parentFolder
            opacity: folderModel.folder != folderModel.parentFolder ? 1.0 : 0.5
        }
        Label {
            platformStyle: LabelStyle {inverted: root.platformStyle.inverted }
            anchors.left: backButton.right
            anchors.leftMargin: 6
            anchors.right: parent.right
            height: backButton.height
            text: url2local(folderModel.folder)
            elide: Text.ElideLeft
            verticalAlignment: Text.AlignVCenter
        }
    }

    FolderListModel {
        id: folderModel
        folder: local2url(root.folder)
        nameFilters: splitFilters(root.nameFilters)
        property string selectedFile: ""

        onFolderChanged: {
            if (__folderSet) {
                __folderSet = false;
            } else {
                __folderChanged = true
                root.folder = url2local(folder)
            }
        }

        function splitFilters(filters) {
            if (!filters) { return [] }
            var length = filters.length
            var allPatterns = []
            for (var i = 0; i < length; i++) {
                var filter = filters[i]
                var filePatterns = filter.match(/\(([^)]*)\)/)
                if (filePatterns) {
                    filePatterns = filePatterns[1]
                } else {
                    filePatterns = filter
                }
                allPatterns = allPatterns.concat(filePatterns.split(" "))
            }
            return allPatterns
        }
    }

    ListView {
        anchors.top: toolBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true

        Component {
            id: fileDelegate
            Rectangle {
                color: filePath == folderModel.selectedFile ?
                    root.platformStyle.itemSelectedBackgroundColor :
                    root.platformStyle.itemBackgroundColor
                width: parent.width
                height: root.platformStyle.itemHeight
                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    height: parent.height
                    width: height
                    source: "image://theme/icon-m-toolbar-directory" + __iconStyle
                    visible: folderModel.isFolder(index)
                }
                Text {
                    text: fileName
                    font: root.platformStyle.itemFont
                    color: root.platformStyle.itemTextColor
                    anchors.fill: parent
                    anchors.leftMargin: parent.height
                    verticalAlignment: Text.AlignVCenter
                }
                MouseArea {
                    anchors.fill: parent
                    onPressed: folderModel.selectedFile = filePath
                    onClicked: {
                        if (folderModel.isFolder(index)) {
                            folderModel.folder = filePath
                        } else {
                            root.filePath = url2local(filePath)
                        }
                    }
                }
            }
        }

        model: folderModel
        delegate: fileDelegate
    }

    function local2url(localPath) {
        if (localPath.substring(0, 7) !== "file://") {
            return "file://" + localPath
        } else {
            return localPath
        }
    }

    function url2local(url) {
        var s = url.toString()
        if (s.substring(0, 7) === "file://") {
            return s.substring(7)
        } else {
            return s
        }
    }

}

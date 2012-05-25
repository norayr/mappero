import QtQuick 1.0
import com.nokia.meego 1.0
import Qt.labs.folderlistmodel 1.0

Item {
    id: root

    property bool selectFolder: false
    property bool selectMultiple: false
    property string filePath: ""
    property alias folder: folderModel.folder
    property alias nameFilters: folderModel.nameFilters

    property Style platformStyle: FileBrowserStyle {}

    property string __iconStyle: platformStyle.inverted ? "-white" : ""

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
            text: folderModel.folder
            elide: Text.ElideLeft
            verticalAlignment: Text.AlignVCenter
        }
    }

    FolderListModel {
        property string selectedFile: ""
        id: folderModel
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
                            root.filePath = filePath
                        }
                    }
                }
            }
        }

        model: folderModel
        delegate: fileDelegate
    }
}

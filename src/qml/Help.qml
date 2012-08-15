import QtQuick 1.0

Item {
    id: root

    signal close()

    width: 600
    focus: true
    Keys.onPressed: if (event.modifiers == Qt.NoModifier) close()

    MouseArea {
        anchors.fill: parent
        onClicked: root.close()
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 20
        radius: 10
        color: "white"
        border.width: 1

        Column {
            id: title
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 15
            spacing: 10

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    font.bold: true
                    font.pointSize: 20
                    horizontalAlignment: Text.AlignHCenter
                    text: "Mappero Geotagger"
                }
                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    font.pointSize: 8
                    horizontalAlignment: Text.AlignHCenter
                    textFormat: Text.StyledText
                    text: qsTr("version ") + "<b>1.0</b>"
                }
            }

            Text {
                anchors.left: parent.left
                anchors.right: parent.right
                font.pointSize: 8
                horizontalAlignment: Text.AlignHCenter
                textFormat: Text.StyledText
                text: qsTr("Written by Alberto Mardegan and distributed under the <b>GPLv3</b> license")
            }

            Item { width: 1; height: 10 }

            Text {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 15
                font.pointSize: 10
                wrapMode: Text.WordWrap
                text: qsTr("If you got this program for free, please consider <a href=\"http://www.mardy.it/mappero-geotagger\">making a donation</a> to support its development.\nFor any questions, feature requests or bug reports please <a href=\"http://lists.mardy.it/listinfo.cgi/mappero-geotagger-mardy.it\">join the mailing-list</a>.")
                onLinkActivated: Qt.openUrlExternally(link)
            }
        }

        Item {
            anchors.top: title.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 15

            Item {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.right: parent.right
                height: keybindingTitle.height + grid.height
                Text {
                    id: keybindingTitle
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 10
                    font.bold: true
                    font.pointSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Key bindings")
                }

                Grid {
                    id: grid
                    anchors.top: keybindingTitle.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.topMargin: 10
                    columns: 2
                    spacing: 4
                    Repeater {
                        model: keysModel
                        delegate: Row {
                            spacing: 8
                            Text {
                                width: 50
                                font.bold: true
                                horizontalAlignment: Text.AlignRight
                                text: model.key
                            }
                            Text {
                                width: 200
                                wrapMode: Text.WordWrap
                                text: qsTr(model.label)
                            }
                        }
                    }
                }
            }
        }

        ListModel {
            id: keysModel
            ListElement { key: "R"; label: QT_TR_NOOP("Revert the selected file(s) to the original location") }
            ListElement { key: "S"; label: QT_TR_NOOP("Save the changes to the selected file(s)") }
            ListElement { key: "X"; label: QT_TR_NOOP("Save the changes to the selected file(s) and close them") }
            ListElement { key: "D"; label: QT_TR_NOOP("Delete the location information from the selected file(s)") }
            ListElement { key: "C"; label: QT_TR_NOOP("Close the selected file(s)") }
            ListElement { key: "Ctrl+A"; label: QT_TR_NOOP("Select all") }
            ListElement { key: "H"; label: QT_TR_NOOP("Show this help screen") }
        }
    }
}

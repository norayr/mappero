import QtQuick 2.0

Item {
    id: root

    signal close()

    width: 600
    focus: true
    Keys.onPressed: if (event.modifiers == Qt.NoModifier) close()

    MouseArea {
        x: 0
        y: 0
        width: mainWindow.width
        height: mainWindow.height
        onWheel: wheel.accepted = true
        onClicked: root.close()
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 20
        radius: 10
        color: "white"
        border.width: 1

        Flickable {
            id: flickable
            anchors.fill: parent
            anchors.margins: 15
            contentHeight: contentColumn.height
            clip: true

            Column {
                id: contentColumn
                anchors.left: parent.left
                anchors.right: parent.right
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
                        text: qsTr("version <b>%1</b>").arg(Qt.application.version)
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

                Item { width: 1; height: 10 }

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 15
                    font.pointSize: 10
                    wrapMode: Text.WordWrap
                    textFormat: Text.RichText
                    text: qsTr(
`<center><h2>Quick help</h2></center>
<p>To add geolocation information to your photos, please follow these steps:</p>
<ol>
  <li>Drop the photos onto the white area at the bottom of the window
  <li>Click on the photo(s) to geotag, then drag the orange marker and drop it
  on the map, at the desired location
  <li>Click on the \"Save\" button
</ol>
<p>You can also correlate with a GPX route, by selecting one via the <img
src=\"qrc:correlate\" width=\"20\"/> button.  The images should automatically
be placed along the route, and the time slider on the upper left corner of the
screen can be used to synchronise the images with the track.</p>
<p>There's also <a href=\"https://www.youtube.com/watch?v=b1J84dISuNk\">a video
showcasing Mappero Geotagger</a>, where the various features are briefly shown
in action.</p>
`)
                    onLinkActivated: Qt.openUrlExternally(link)
                }

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
            ListElement { key: "N"; label: QT_TR_NOOP("Select the next file which has not been assigned a location") }
            ListElement { key: "R"; label: QT_TR_NOOP("Revert the selected file(s) to the original location") }
            ListElement { key: "S"; label: QT_TR_NOOP("Save the changes to the selected file(s)") }
            ListElement { key: "X"; label: QT_TR_NOOP("Save the changes to the selected file(s) and close them") }
            ListElement { key: "D"; label: QT_TR_NOOP("Delete the location information from the selected file(s)") }
            ListElement { key: "C"; label: QT_TR_NOOP("Close the selected file(s)") }
            ListElement { key: "Ctrl+A"; label: QT_TR_NOOP("Select all files") }
            ListElement { key: "H"; label: QT_TR_NOOP("Show this help screen") }
        }
    }
}

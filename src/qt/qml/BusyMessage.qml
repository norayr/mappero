import QtQuick 2.0
import "UIConstants.js" as UI

Rectangle {
    property int count: 0
    property bool gotCloseRequest: false

    anchors.right: parent.right
    anchors.bottom: parent.bottom
    radius: UI.ToolbarMargins / 2
    color: "red"
    visible: gotCloseRequest && count > 0
    height: saveLabel.height + UI.ToolbarMargins
    width: saveLabel.width + UI.ToolbarMargins

    onCountChanged: if (count === 0) gotCloseRequest = false

    Text {
        id: saveLabel

        anchors.centerIn: parent
        text: qsTr("Busy saving %n element(s)", "", count)
    }
}

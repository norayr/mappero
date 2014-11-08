import QtQuick 2.0
import "UIConstants.js" as UI

Rectangle {
    property int count: 0

    anchors.right: parent.right
    anchors.bottom: parent.bottom
    radius: UI.ToolbarMargins / 2
    color: "red"
    visible: count > 0
    height: saveLabel.height + UI.ToolbarMargins
    width: saveLabel.width + UI.ToolbarMargins

    Text {
        id: saveLabel

        anchors.centerIn: parent
        text: qsTr("Saving %n element(s)", "", count)
    }
}

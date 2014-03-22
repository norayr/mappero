import QtQuick 2.0
import "UIConstants.js" as UI

Item {
    id: root

    property alias text: textItem.text

    signal clicked

    width: parent.width
    height: UI.MenuButtonHeight

    Text {
        id: textItem

        color: root.enabled ? UI.TextColor : UI.TextColorDisabled
        anchors.centerIn: parent
        font.pointSize: UI.MenuButtonFontPixelSize
    }

    MouseArea {
        anchors.fill: parent
        onClicked: parent.clicked()
    }
}

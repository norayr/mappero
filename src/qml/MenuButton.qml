import QtQuick 2.0
import "UIConstants.js" as UI

Item {
    property alias text: textItem.text

    signal clicked

    width: parent.width
    height: UI.MenuButtonHeight

    Text {
        id: textItem

        anchors.centerIn: parent
        font.pointSize: UI.MenuButtonFontPixelSize
    }

    MouseArea {
        anchors.fill: parent
        onClicked: parent.clicked()
    }
}

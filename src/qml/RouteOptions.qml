import QtQuick 2.0
import Mappero 1.0

Popup {
    id: root

    property var plugin
    property alias optionsData: optionsLoader.options

    minimumWidth: optionsLoader.item.implicitWidth
    minimumHeight: optionsLoader.item.implicitHeight
    position: "pageCenter"

    Text {
        id: titleText
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Options for %1").arg(plugin.displayName)
    }

    Loader {
        id: optionsLoader
        property var options
        anchors.top: titleText.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        sourceComponent: plugin.optionsUi
    }
}

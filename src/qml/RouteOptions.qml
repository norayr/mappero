import QtQuick 2.0
import Mappero 1.0

Popup {
    id: root

    property alias optionsComponent: optionsLoader.sourceComponent
    property alias optionsData: optionsLoader.options

    minimumWidth: optionsLoader.item.implicitWidth
    minimumHeight: optionsLoader.item.implicitHeight
    position: "pageCenter"

    Loader {
        id: optionsLoader
        property var options
        anchors.fill: parent
    }
}

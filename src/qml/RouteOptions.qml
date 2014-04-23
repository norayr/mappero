import QtQuick 2.0
import Mappero 1.0

Popup {
    id: root

    property alias optionsComponent: optionsLoader.sourceComponent

    minimumWidth: optionsLoader.item.implicitWidth
    minimumHeight: optionsLoader.item.implicitHeight
    position: "pageCenter"

    Loader {
        id: optionsLoader
        anchors.fill: parent
    }
}

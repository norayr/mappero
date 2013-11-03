import QtQuick 2.0
import Mappero 1.0

ImageButton {
    id: root

    property string activePlugin: model.get(__selectedIndex, "name")
    property int __selectedIndex: 0
    property variant model

    source: model.get(__selectedIndex, "icon")

    onClicked: {
        if (selector.isOpen) {
            selector.close()
        } else {
            selector.open()
        }
    }

    SearchPluginSelector {
        id: selector
        parent: root.parent
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.bottom
        anchors.margins: 10
        model: root.model
        selectedIndex: __selectedIndex
        source: root
        onIsOpenChanged: if (!isOpen) { __selectedIndex = selectedIndex; }
    }
}

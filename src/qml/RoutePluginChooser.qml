import QtQuick 2.0
import Mappero 1.0

Button {
    id: root

    property string activePlugin: model.get(__selectedIndex, "name")
    property int __selectedIndex: 0
    property variant model

    text: model.get(__selectedIndex, "displayName")
    spacing: 2
    labelAlignment: Text.AlignLeft
    rightItems: [
        Image {
            height: parent.height
            width: height
            source: model.get(__selectedIndex, "icon")
        }
    ]

    onClicked: {
        if (loader.item && loader.item.isOpen) {
            loader.item.close()
        } else {
            loader.setSource("PluginSelector.qml", {
                "parent": root,
                "anchors.horizontalCenter": root.horizontalCenter,
                "anchors.top": root.bottom,
                "anchors.margins": 10,
                "model": root.model,
                "selectedIndex": __selectedIndex,
                "source": root
            })
            loader.item.open()
        }
    }

    Loader {
        id: loader
    }

    Connections {
        target: loader.item
        onIsOpenChanged: if (!loader.item.isOpen) __selectedIndex = loader.item.selectedIndex
    }
}

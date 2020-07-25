import QtQuick 2.0
import Mappero.Ui 1.0

ImageButton {
    id: root

    property string activePlugin: model.get(__selectedIndex, "name")
    property int __selectedIndex: preferredPluginIndex()
    property var model: null

    source: model.get(__selectedIndex, "icon")

    onClicked: {
        if (loader.item && loader.item.isOpen) {
            loader.item.close()
        } else {
            loader.setSource("PluginSelector.qml", {
                "parent": root.parent,
                "anchors.horizontalCenter": root.parent.horizontalCenter,
                "anchors.top": root.parent.bottom,
                "anchors.margins": 10,
                "model": root.model,
                "selectedIndex": __selectedIndex,
                "source": root
            })
            loader.item.open()
        }
    }
    onActivePluginChanged: Mappero.conf.preferredSearchPlugin = activePlugin

    Loader {
        id: loader
    }

    Connections {
        target: loader.item
        onIsOpenChanged: if (!loader.item.isOpen) __selectedIndex = loader.item.selectedIndex
    }

    function preferredPluginIndex() {
        var pluginName = Mappero.conf.preferredSearchPlugin
        var model = root.model
        for (var i = 0; i < model.count; i++) {
            var name = model.get(i, "name")
            if (name == pluginName) return i
        }
        return 0
    }
}

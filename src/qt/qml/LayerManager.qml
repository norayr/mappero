import QtQuick 2.0
import Mappero 1.0

Item {
    id: root

    property var mainLayer: root.createLayer(mainLayerIndex)
    property int mainLayerIndex: layerModel.find(Controller.conf.lastMainLayer)
    property var model: layerModel

    property var __currentLayer: null
    property int __currentIndex: -1

    TiledMaps {
        id: layerModel
    }

    function createLayer(index) {
        if (index != __currentIndex || !__currentLayer) {
            __currentIndex = index
            __currentLayer = tiledLayer.createObject(root, layerModel.get(index))
        }
        return __currentLayer
    }

    Component {
        id: tiledLayer
        TiledLayer {}
    }

    Connections {
        target: mainWindow
        onClosing: {
            Controller.conf.lastMainLayer = mainLayer.uid
        }
    }

    function setLayer(index) {
        mainLayerIndex = index
    }
}

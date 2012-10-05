import QtQuick 1.1
import Mappero 1.0

Item {
    id: root

    property variant mainLayer: tiledLayer.createObject(root, layerModel.get(mainLayerIndex))
    property int mainLayerIndex: layerModel.find(Mappero.conf.lastMainLayer)
    property variant model: layerModel

    TiledMaps {
        id: layerModel
    }

    Component {
        id: tiledLayer
        TiledLayer {}
    }

    Connections {
        target: view
        onClosing: {
            Mappero.conf.lastMainLayer = mainLayer.uid
        }
    }

    function setLayer(index) {
        mainLayerIndex = index
    }
}
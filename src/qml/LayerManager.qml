import QtQuick 1.1
import Mappero 1.0

Item {
    id: root

    property variant mainLayer: tiledLayer.createObject(root, layerModel.get(0))

    TiledMaps {
        id: layerModel
    }

    Component {
        id: tiledLayer
        TiledLayer {}
    }
}

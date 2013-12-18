import QtQuick 2.0
import Mappero 1.0

ListBrowser {
    id: root

    property variant currentGeoPoint

    delegate: PoiBrowserDelegate {
        width: ListView.view.width
        height: ListView.view.height
        name: model.name
        address: model.address
        ListView.onIsCurrentItemChanged: {
            if (ListView.isCurrentItem) root.currentGeoPoint = model.geoPoint
        }
    }
}

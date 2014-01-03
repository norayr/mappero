import QtQuick 2.0
import Mappero 1.0

ListBrowser {
    id: root

    property variant currentGeoPoint

    signal destinationSet(variant point)
    signal originSet(variant point)

    delegate: PoiBrowserDelegate {
        id: browserDelegate
        width: ListView.view.width
        height: ListView.view.height
        name: model.name
        address: model.address
        ListView.onIsCurrentItemChanged: {
            if (ListView.isCurrentItem) root.currentGeoPoint = model.geoPoint
        }
        onClicked: {
            loader.setSource("PoiActions.qml", {
                "source": browserDelegate
            })
            loader.item.open()
        }
    }

    Loader {
        id: loader

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.top
        anchors.margins: 10
    }

    Connections {
        target: loader.item
        onDestinationSet: root.destinationSet(currentPoint())
        onOriginSet: root.originSet(currentPoint())
    }

    function currentPoint() {
        var point = {
            "geoPoint": model.get(currentIndex, "geoPoint"),
            "name": model.get(currentIndex, "name")
        }
        return point
    }
}

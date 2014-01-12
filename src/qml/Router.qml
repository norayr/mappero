import QtQuick 2.0
import Mappero 1.0

Item {
    id: root

    property variant currentPosition
    property variant destinationPoint
    property string destinationName
    property variant originPoint
    property string originName
    property variant model: null
    property int currentIndex: -1
    readonly property bool isOpen: loader.status == Loader.Ready
    readonly property bool browsing: __routeBrowser != null
    readonly property int routeEndsModel: Mappero.isValid(originPoint) || Mappero.isValid(destinationPoint)
    property variant routeEndsView: Component {
        PoiView {
            anchors.fill: parent
            model: endsModel
            delegate: endsDelegate
        }
    }

    property variant __routeBrowser: null
    property bool routeBrowserNeeded: (model != null) && (model.count > 0)

    anchors.fill: parent

    onRouteBrowserNeededChanged: {
        if (routeBrowserNeeded) {
            var component = Qt.createComponent("RouteBrowser.qml")
            __routeBrowser = component.createObject(bottomPanel, {
                "model": model
            })
            routeBrowserConnection.target = __routeBrowser
            root.currentIndex = 0
        } else {
            if (__routeBrowser) __routeBrowser.destroy()
            routeBrowserConnection.target = null
            __routeBrowser = null
            root.currentIndex = -1
        }
    }

    PoiModel {
        id: endsModel
        roles: [ "name" ]
    }

    Component {
        id: endsDelegate
        PoiItem {
            width: 64
            height: 64
            Image {
                anchors.fill: parent
                source: "qrc:/flag"
            }
            Text {
                x: parent.width / 2 + 2
                y: 4
                text: model.index + 1
            }
        }
    }
    onDestinationPointChanged: updateEnds()
    onOriginPointChanged: updateEnds()

    function updateEnds() {
        endsModel.clear()
        endsModel.append({
            "geoPoint": originPoint,
            "name": originName
        })
        endsModel.append({
            "geoPoint": destinationPoint,
            "name": destinationName
        })
    }

    Loader {
        id: loader
    }

    Connections {
        target: loader.item
        onRoutesReady: {
            model = loader.item.routeModel
            loader.item.close()
        }
    }

    Connections {
        id: routeBrowserConnection
        target: null
        onCurrentIndexChanged: root.currentIndex = __routeBrowser.currentIndex
    }

    onCurrentPositionChanged: {
        if (loader.item) loader.item.currentPosition = currentPosition
    }

    function open() {
        console.log("currentPosition: " + currentPosition + " isValid: " + Mappero.isValid(currentPosition))
        loader.setSource("RouterDialog.qml", {
            "currentPosition": currentPosition,
            "destinationPoint": destinationPoint,
            "destinationName": destinationName,
            "originPoint": originPoint,
            "originName": originName,
            "source": root,
            "position": "pageCenter"
        })
        loader.item.open()
    }

    function stopBrowsing() {
        root.model = null
        open()
    }
}

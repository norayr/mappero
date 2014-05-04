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
    readonly property bool isOpen: loader.status == Loader.Ready && loader.item.isOpen
    readonly property bool browsing: __routeBrowser != null
    readonly property int routeEndsModel: Mappero.isValid(originPoint) || Mappero.isValid(destinationPoint)
    property variant routeEndsView: Component {
        PoiView {
            anchors.fill: parent
            model: endsModel
            delegate: endsDelegate
        }
    }

    property var path
    property Component pathWayPointDelegate
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
            /* HACK: wait for the bottomPanel to appear before setting the
             * currentIndex, or map.lookAt() could be considering a bigger map
             * than what we'll end up having once the bottomPanel appears */
            currentIndexTimer.start()
        } else {
            if (__routeBrowser) __routeBrowser.destroy()
            routeBrowserConnection.target = null
            __routeBrowser = null
            root.currentIndex = -1
        }
    }

    Timer {
        id: currentIndexTimer
        onTriggered: currentIndex = 0
        running: false
        interval: 200
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
        onRouteSet: {
            root.pathWayPointDelegate = route.wayPointDelegate
            root.path = route.path
            root.model.clear()
            loader.sourceComponent = null
        }
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

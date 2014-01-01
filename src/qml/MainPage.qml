import QtQuick 2.0
import Mappero 1.0

Item {
    anchors.fill: parent
    property bool __isPage: true

    LayerManager {
        id: layerManager
    }

    MapFlickable {
        id: mapFlickable
        anchors.fill: parent

        PinchArea {
            anchors.fill: parent
            onPinchStarted: mapFlickable.interactive = false
            onPinchUpdated: map.pinchScale = pinch.scale
            onPinchFinished: {
                map.pinchScale = 0
                mapFlickable.interactive = true
            }
        }
    }

    Map {
        id: map
        anchors.fill: parent
        flickable: mapFlickable

        mainLayer: layerManager.mainLayer
        center: Mappero.conf.lastPosition
        requestedZoomLevel: Mappero.conf.lastZoomLevel
        followGps: visible

        PathLayer {
            id: pathLayer

            Tracker {
                id: tracker
                tracking: true
                color: "red"
            }

            RouteItems {
                id: routeItemRepeater
                model: router.model
                currentIndex: router.currentIndex
            }
        }

        WayPointView {
            model: pathLayer.items
        }

        PoiView {
            id: searchView
            anchors.fill: parent
            delegate: searchBox.delegate
            model: searchBox.model
        }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 4
        color: "white"
        opacity: 0.8
        width: 100
        height: content.height + 8
        visible: !tracker.empty

        Column {
            id: content
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 4
            Text {
                text: Mappero.formatLength(tracker.length)
            }
        }
    }

    Osm {
        id: osm
        anchors.fill: parent
        tracker: tracker
        searchBox: searchBox
    }

    Connections {
        target: view
        onClosing: {
            Mappero.conf.lastPosition = map.center
            Mappero.conf.lastZoomLevel = map.zoomLevel
            Mappero.conf.gpsInterval = gps.updateInterval
        }
    }

    PoiBrowser {
        model: searchBox.model
        onCurrentGeoPointChanged: map.requestedCenter = currentGeoPoint
        onDestinationSet: {
            router.destinationPoint = model.get(currentIndex, "geoPoint")
            router.destinationName = model.get(currentIndex, "name")
            router.open()
            model.clear()
        }
        onOriginSet: {
            router.originPoint = model.get(currentIndex, "geoPoint")
            router.originName = model.get(currentIndex, "name")
            model.clear()
        }
    }

    Router {
        id: router
        currentPosition: map.center // TODO: use GPS!
        onCurrentIndexChanged: map.lookAt(routeItemRepeater.itemAt(currentIndex).itemArea(), 0, 0, 40)
    }

    SearchBox {
        id: searchBox
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: 200 * Mappero.uiScale
        location: Mappero.point(map.center)
        visible: false
    }

}

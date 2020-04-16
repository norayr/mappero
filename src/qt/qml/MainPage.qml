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
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: bottomPanel.top
        }
        flickable: mapFlickable

        mainLayer: layerManager.mainLayer
        center: Mappero.conf.lastPosition
        requestedZoomLevel: Mappero.conf.lastZoomLevel
        followGps: visible

        PathLayer {
            id: pathLayer

            PathItem {
                id: route
                color: "green"
            }

            Tracker {
                id: tracker
                tracking: true
                color: "red"
            }

            RouteItems {
                id: routeItemRepeater
                model: router.model
                currentIndex: router.currentIndex
                onCurrentItemChanged: if (currentItem) map.lookAt(currentItem.itemArea(), 0, 0, 10, 10)
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

        Repeater {
            model: router.routeEndsModel
            delegate: router.routeEndsView
        }
    }

    Item {
        id: bottomPanel
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: childrenRect.height

        Behavior on height {
            SmoothedAnimation { duration: 200 }
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
        router: router
        route: route
        tracker: tracker
        searchBox: searchBox
    }

    Connections {
        target: mainWindow
        onClosing: {
            Mappero.conf.lastPosition = map.center
            Mappero.conf.lastZoomLevel = map.zoomLevel
            Mappero.conf.gpsInterval = gps.updateInterval
        }
    }

    Router {
        id: router
        currentPosition: map.center // TODO: use GPS!
        onPathChanged: {
            route.path = path
            route.wayPointDelegate = pathWayPointDelegate
        }
    }

    SearchBox {
        id: searchBox
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: 200 * Mappero.uiScale
        location: Mappero.point(map.center)
        visible: false

        onCurrentGeoPointChanged: map.requestedCenter = currentGeoPoint
        onDestinationSet: {
            console.log("Destination set")
            router.destinationPoint = point.geoPoint
            router.destinationName = point.name
            router.open()
            model.clear()
        }
        onOriginSet: {
            console.log("Origin set")
            router.originPoint = point.geoPoint
            router.originName = point.name
            model.clear()
        }
    }
}

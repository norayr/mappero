import QtQuick 2.0

Item {
    id: root

    property variant currentPosition
    property variant destinationPoint
    property variant destinationName
    property variant originPoint
    property variant originName
    property variant model: null
    property int currentIndex: -1
    readonly property bool isOpen: loader.status == Loader.Ready
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
}

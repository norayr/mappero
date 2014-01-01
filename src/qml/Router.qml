import QtQuick 2.0

Item {
    id: root

    property variant currentPosition
    property variant destinationPoint
    property variant destinationName
    property variant originPoint
    property variant originName
    property variant model
    property int currentIndex

    anchors.fill: parent

    Loader {
        id: loader
    }

    Loader {
        id: browserLoader
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
            margins: 2
        }
    }

    Connections {
        target: loader.item
        onRoutesReady: {
            model = loader.item.routeModel
            loader.item.close()
            browserLoader.setSource("RouteBrowser.qml", {
                "model": model
            })
        }
    }

    Connections {
        target: browserLoader.item
        onCurrentIndexChanged: root.currentIndex = browserLoader.item.currentIndex
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

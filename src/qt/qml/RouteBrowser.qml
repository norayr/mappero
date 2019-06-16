import QtQuick 2.0
import Mappero.Ui 1.0

ListBrowser {
    id: root

    signal routeSet(var route)

    delegate: Item {
        id: listDelegate
        property Item routeDelegate: null
        width: ListView.view.width
        height: ListView.view.height
        Component.onCompleted: {
            routeDelegate = root.model.routeDelegate.createObject(listDelegate, {
                "path": model.path
            })
        }

        Connections {
            target: routeDelegate
            onClicked: {
                loader.setSource("RouteActions.qml", {
                    "source": listDelegate,
                    "path": routeDelegate.pathItem
                })
                loader.item.open()
            }
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
        onActivated: root.routeSet(currentRoute())
    }

    function currentRoute() {
        var route = {
            "path": model.get(currentIndex, "path"),
            "wayPointDelegate": model.wayPointDelegate
        }
        return route
    }
}

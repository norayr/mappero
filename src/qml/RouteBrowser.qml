import QtQuick 2.0
import Mappero 1.0

ListBrowser {
    id: root

    delegate: Item {
        id: listDelegate
        width: ListView.view.width
        height: ListView.view.height
        Component.onCompleted: {
            var subItem = root.model.routeDelegate.createObject(listDelegate, {
                "path": model.path
            })
        }
    }
}

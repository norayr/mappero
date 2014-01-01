import QtQuick 2.0
import Mappero 1.0

Item {
    id: root

    property alias path: pathItemInternal.path
    property Item pathItem: pathItemInternal

    signal clicked()

    anchors.fill: parent

    PathItem {
        id: pathItemInternal
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.clicked()
    }
}

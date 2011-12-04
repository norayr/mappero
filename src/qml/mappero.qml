import QtQuick 1.0

Rectangle {
    id: screen
    width: 800
    height: 480

    Osm {
        id: osm
        anchors.fill: parent
    }
}

import QtQuick 2.0

Image {
    id: root

    property bool running: false

    anchors.centerIn: parent
    visible: running
    source: "qrc:/busy-indicator"

    RotationAnimation on rotation {
        loops: Animation.Infinite
        from: 0
        to: 360
        running: root.running
        duration: 5000
    }

}

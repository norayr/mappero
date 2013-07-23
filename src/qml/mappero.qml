import QtQuick 2.0
import Mappero 1.0

Rectangle {
    width: 800
    height: 480

    Loader {
        anchors.fill: parent
        focus: true
        source: firstPage
    }
}

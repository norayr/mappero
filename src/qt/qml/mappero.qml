import QtQuick 2.0
import QtQuick.Window 2.1
import Mappero 1.0

Window {
    id: mainWindow
    width: 800
    height: 480
    visible: true
    title: qsTr("Mappero")

    Loader {
        id: loader
        anchors.fill: parent
        focus: true
        source: application.firstPage
    }
}

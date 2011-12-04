import QtQuick 1.0

Image {
    signal clicked

    MouseArea {
        anchors.fill: parent
        onClicked: parent.clicked()
    }
}

import QtQuick 2.0
import Mappero 1.0

Flickable {
    id: root
    flickableDirection: Flickable.HorizontalAndVerticalFlick
    property double neutralX: 400000
    property double neutralY: 400000
    property var position: Qt.point(0, 0)
    property bool _initialized: false

    contentWidth: neutralX * 2
    contentHeight: neutralY * 2
    contentX: neutralX
    contentY: neutralY
    Component.onCompleted: _initialized = true

    signal panFinished

    onMovementEnded: {
        panFinished()
        contentX = neutralX
        contentY = neutralY
    }

    Binding {
        property: "position"
        target: root
        value: Qt.point(contentX - neutralX, contentY - neutralY)
        when: root._initialized
    }
}

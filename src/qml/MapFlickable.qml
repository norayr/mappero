import QtQuick 1.0
import Mappero 1.0

Flickable {
    flickableDirection: Flickable.HorizontalAndVerticalFlick
    property double neutralX: 400000
    property double neutralY: 400000
    property variant position: Qt.point(contentX - neutralX,
                                        contentY - neutralY)
    contentWidth: neutralX * 2
    contentHeight: neutralY * 2
    contentX: neutralX
    contentY: neutralY

    signal panFinished

    onMovementEnded: {
        panFinished()
        contentX = neutralX
        contentY = neutralY
    }
}

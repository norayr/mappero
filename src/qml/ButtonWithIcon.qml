import Mappero 1.0
import QtQuick 2.0

Button {
    id: root

    property var iconSource

    spacing: 2
    labelAlignment: Text.AlignLeft
    rightItems: Image {
        height: parent.height
        width: height
        source: iconSource
        visible: iconSource
    }
}

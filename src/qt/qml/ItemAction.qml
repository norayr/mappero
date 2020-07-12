import QtQuick 2.0

Item {
    id: root

    property var selectedItems: []
    property url source

    signal activate(var taggable)

    enabled: selectedItems.length > 0

    Image {
        anchors.fill: parent
        source: enabled ? root.source : root.source + "-off"
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            var l = selectedItems.length
            for (var i = 0; i < l; i++) {
                var taggable = selectedItems[i]
                root.activate(taggable)
            }
        }
    }
}

import QtQuick 1.0

Item {
    id: root

    property variant selectedItems: []

    enabled: selectedItems.length > 0

    Image {
        anchors.fill: parent
        source: enabled ? ":taggable-reload.svg" : ":taggable-reload-off.svg"
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            var l = selectedItems.length
            for (var i = 0; i < l; i++) {
                var taggable = selectedItems[i]
                taggable.reload()
            }
        }
    }
}

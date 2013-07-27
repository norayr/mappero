import QtQuick 1.0

Item {
    id: root

    property variant selectedItems: []

    signal dropped(variant pos)

    enabled: selectedItems.length > 0

    Item {
        id: handle
        property real hotX: width / 2
        property real hotY: height
        property variant _previews: []

        width: parent.width
        height: parent.height
        Image {
            id: movingHandle
            anchors.fill: parent
            source: enabled ? "qrc:taggable-handle" : "qrc:taggable-handle-off"
        }
        Image {
            id: cross
            x: handle.hotX - width / 2
            y: handle.hotY - height / 2
            visible: handleMouseArea.drag.active
            source: "qrc:taggable-handle-in"
        }

        states: [
            State {
                name: ""
                PropertyChanges { target: handle; x: 0; y: 0 }
            },
            State {
                name: "dragging"
                when: handleMouseArea.drag.active
                PropertyChanges { target: movingHandle; opacity: 0.8 }
                PropertyChanges {
                    target: previews;
                    model: selectedItems.slice(0, 5)
                }
            }
        ]

        Repeater {
            id: previews
            Image {
                x: 20 + 10 * index
                y: -height -10 * index
                z: -index
                width: 40
                height: 40
                fillMode: Image.PreserveAspectCrop
                clip: true
                source: modelData.pixmapUrl
                sourceSize.width: width
                sourceSize.height: height
                opacity: {
                    var o = 1.0
                    for (var i=0;i<index;i++) { o *= 0.5 }
                    return o
                }
            }
        }
    }

    MouseArea {
        id: handleMouseArea

        anchors.fill: parent
        drag.target: handle
        drag.onActiveChanged: {
            if (!drag.active) {
                root.dropped(root.mapToItem(null,
                    handle.x + handle.hotX,
                    handle.y + handle.hotY))
            }
        }
    }
}

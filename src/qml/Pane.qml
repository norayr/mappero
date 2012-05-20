import QtQuick 1.0

Item {
    id: source
    default property alias content: contentItem.children

    Item {
        id: activePane
        parent: getRoot()
        anchors.fill: parent
        state: "closed"

        states: [
            State {
                name: "closed"
                PropertyChanges { target: activePane; visible: false; }
            }
        ]

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.5
        }

        MouseArea {
            anchors.fill: parent
            onClicked: source.close()
        }

        Rectangle {
            id: pane
            anchors.centerIn: parent
            width: contentItem.childrenRect.width
            height: contentItem.childrenRect.height
            color: "white"

            MouseArea {
                anchors.fill: parent
            }

            Item {
                id: contentItem
            }
        }

        function getRoot() {
            var root = source.parent
            while (root.parent)
            {
                root = root.parent
            }
            return root;
        }
    }

    function open() {
        activePane.state = "open"
    }

    function close() {
        activePane.state = "closed"
    }
}

import QtQuick 2.0
import Mappero 1.0

Item {
    id: root

    property alias model: listView.model
    property alias currentIndex: listView.currentIndex
    property alias delegate: listView.delegate

    anchors {
        top: parent.top
        left: parent.left
        right: parent.right
    }
    height: 100 * Mappero.uiScale
    visible: model && model.count > 0

    Rectangle {
        anchors.fill: parent
        color: "white"
    }

    ImageButton {
        id: prev
        anchors {
            bottom: parent.bottom
            top: parent.top
            left: parent.left
        }
        width: height / 2
        source: "qrc:/arrow-left"
        useAlternateWhenDisabled: false
        onClicked: listView.decrementCurrentIndex()
        enabled: listView.currentIndex != 0
    }

    ListView {
        id: listView
        anchors {
            bottom: parent.bottom
            top: parent.top
            left: prev.right
            right: next.left
        }

        clip: true
        orientation: ListView.Horizontal
        snapMode: ListView.SnapToItem
    }

    ImageButton {
        id: next
        anchors {
            bottom: parent.bottom
            top: parent.top
            right: parent.right
        }
        width: height / 2
        source: "qrc:/arrow-right"
        useAlternateWhenDisabled: false
        onClicked: listView.incrementCurrentIndex()
        enabled: listView.currentIndex != listView.model.count - 1
    }
}

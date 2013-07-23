import QtQuick 2.0

Item {
    id: root

    property variant track
    property variant selection

    property string _fontFamily: "Helvetica"
    property real _fontSize: 8

    width: 200
    height: controls.height + 2 * controls.anchors.margins
    visible: !track.empty

    PaneBackground {}

    Column {
        id: controls
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 4
        spacing: 3

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            font.bold: true
            text: qsTr("GPS track timing")
        }

        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            font.family: root._fontFamily
            font.pointSize: root._fontSize
            text: qsTr("Start:")

            Text {
                anchors.right: parent.right
                font.bold: true
                font.family: root._fontFamily
                font.pointSize: root._fontSize
                horizontalAlignment: Text.AlignRight
                text: Qt.formatDateTime(track.startTime, "d/M/yyyy hh:mm")
            }
        }

        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            font.family: root._fontFamily
            font.pointSize: root._fontSize
            text: qsTr("End:")

            Text {
                anchors.right: parent.right
                font.bold: true
                font.family: root._fontFamily
                font.pointSize: root._fontSize
                horizontalAlignment: Text.AlignRight
                text: Qt.formatDateTime(track.endTime, "d/M/yyyy hh:mm")
            }
        }

        TimeZoneOffset {
            id: timeZone
            anchors.left: parent.left
            anchors.right: parent.right
            onValueChanged: root.updateOffset()
        }

        TimeSlider {
            id: seconds
            anchors.left: parent.left
            anchors.right: parent.right
            onValueChanged: root.updateOffset()
        }
    }

    Connections {
        target: track
        onTimeOffsetChanged: root.updateItems()
        onPathChanged: root.updateItems()
    }

    Connections {
        target: selection
        onItemsChanged: root.updateItems()
    }

    function updateOffset() {
        track.timeOffset = timeZone.value * 3600 + seconds.value
    }

    function updateItems() {
        if (track.empty) return;
        var l = selection.items.length
        for (var i = 0; i < l; i++) {
            var taggable = selection.items[i]
            var position = track.positionAt(taggable.time)
            taggable.setCorrelatedLocation(position)
        }
    }
}

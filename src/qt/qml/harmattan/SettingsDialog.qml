import QtQuick 1.0
import com.nokia.meego 1.0

SelectionDialog {
    id: root

    selectedIndex: indexFromInterval(gps.updateInterval)
    titleText: qsTr("GPS update interval")

    model: ListModel {
        ListElement { name: QT_TR_NOOP("1 second"); value: 1 }
        ListElement { name: QT_TR_NOOP("5 seconds"); value: 5 }
        ListElement { name: QT_TR_NOOP("10 seconds"); value: 10 }
        ListElement { name: QT_TR_NOOP("30 seconds"); value: 30 }
        ListElement { name: QT_TR_NOOP("1 minute"); value: 60 }
        ListElement { name: QT_TR_NOOP("2 minutes"); value: 120 }
        ListElement { name: QT_TR_NOOP("5 minutes"); value: 300 }
    }

    onSelectedIndexChanged: gps.updateInterval = model.get(selectedIndex).value

    function indexFromInterval(interval) {
        for (var i = 0; i < model.count; i++) {
            if (model.get(i).value >= interval) return i
        }
        return 0
    }
}

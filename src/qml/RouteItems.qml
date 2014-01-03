import QtQuick 2.0
import Mappero 1.0

/* This element must be placed inside a PathLayer. It creates PathItem elements
 * for each path defined in the given model. */
Repeater {
    id: root

    property int currentIndex
    property Item currentItem: null

    delegate: PathItem {
        id: pathItem
        path: model.path
        color: "green"
        opacity: root.currentIndex == index ? 1 : 0.2
        z: root.currentIndex == index ? 1 : 0
        wayPointDelegate: root.model.wayPointDelegate
    }
    onItemAdded: if (index == currentIndex) currentItem = item
    onCurrentIndexChanged: currentItem = itemAt(currentIndex)
}

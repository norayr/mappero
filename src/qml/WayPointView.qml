import QtQuick 2.0
import Mappero 1.0

/* The model must be a list of PathItem elements. This component will create
 * PoiViews to visualize the waypoints defined in the paths. */
Repeater {
    delegate: PoiView {
        anchors.fill: parent
        delegate: modelData.wayPointDelegate
        model: modelData.wayPointModel
        opacity: modelData.opacity
        z: modelData.z
    }
}

import QtQuick 2.0
import Mappero 1.0

Rectangle {
    id: root

    property var model: null
    property var delegate
    property var location
    property var span
    property var __plugin: null
    property var __poiBrowser: null
    property var currentGeoPoint

    signal destinationSet(var point)
    signal originSet(var point)

    height: 40 * Mappero.uiScale
    radius: height / 2
    color: "white"
    border {
        width: 2
        color: "grey"
    }

    Connections {
        id: modelConnection
        target: null
        onCountChanged: {
            if (root.model.count > 0) {
                if (__poiBrowser) return
                __poiBrowser = poiBrowserComponent.createObject(bottomPanel, {})
                poiBrowserConnection.target = __poiBrowser
            } else {
                if (__poiBrowser) __poiBrowser.destroy(1000)
                poiBrowserConnection.target = null
                __poiBrowser = null
            }
        }
    }

    Connections {
        id: poiBrowserConnection
        target: null
        onCurrentGeoPointChanged: root.currentGeoPoint = __poiBrowser.currentGeoPoint
        onDestinationSet: root.destinationSet(point)
        onOriginSet: root.originSet(point)
    }

    TextInput {
        id: input
        anchors {
            left: parent.left
            right: icon.left
            verticalCenter: parent.verticalCenter
            leftMargin: root.height / 4
        }
        clip: true
        focus: visible
        font.pixelSize: root.height / 2
        onAccepted: {
            Qt.inputMethod.hide()
            __plugin.query = text
        }
    }

    SearchPluginChooser {
        id: icon
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
            margins: 6
        }
        width: height
        model: PluginManager.pluginModel("search")
        onActivePluginChanged: usePlugin(activePlugin)
    }

    Binding {
        target: __plugin
        property: "location"
        value: location
    }

    Binding {
        target: __plugin
        property: "span"
        value: span
    }

    Component {
        id: poiBrowserComponent
        PoiBrowser {
            model: searchBox.model
            onCurrentGeoPointChanged: map.requestedCenter = currentGeoPoint
        }
    }

    function usePlugin(id) {
        console.log("Activating plugin " + id)
        if (model) model.clear()
        __plugin = PluginManager.loadPlugin(id)
        if (__plugin) {
            __plugin.location = location
            __plugin.span = span
            delegate = __plugin.delegate
            model = __plugin.model
            modelConnection.target = model
        } else {
            modelConnection.target = null
            delegate = null
            model = null
        }
    }
}

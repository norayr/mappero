import QtQuick 2.0
import Mappero 1.0

Rectangle {
    id: root

    property variant model: null
    property variant delegate
    property variant pluginManager
    property variant location
    property variant __plugin: null

    height: 40
    radius: height / 2
    color: "white"
    border {
        width: 2
        color: "grey"
    }

    TextInput {
        id: input
        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
            margins: 4
        }
        focus: true
        onAccepted: __plugin.query = text
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
        model: pluginManager.pluginModel("search")
        onActivePluginChanged: usePlugin(activePlugin)
    }

    Binding {
        target: __plugin
        property: "location"
        value: location
    }

    function usePlugin(id) {
        console.log("Activating plugin " + id)
        if (model) model.clear()
        __plugin = pluginManager.loadPlugin(id)
        if (__plugin) {
            __plugin.location = location
            delegate = __plugin.delegate
            model = __plugin.model
        } else {
            delegate = null
            model = null
        }
    }
}

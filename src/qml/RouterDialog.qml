import QtQuick 2.0
import Mappero 1.0
import "UIConstants.js" as UI

Popup {
    id: root

    property variant currentPosition
    property variant destinationPoint
    property alias destinationName: destinationField.text
    property variant originPoint
    property alias originName: originField.text
    property variant routeModel
    property variant plugin: null

    signal routesReady()

    minimumWidth: 200
    minimumHeight: form.height

    Item {
        anchors.fill: parent

        Column {
            id: form
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: UI.FieldVSpacing

            LabelLayout {
                id: originItem

                text: qsTr("From")

                TextField {
                    id: originField
                }
            }

            LabelLayout {
                id: destinationItem

                text: qsTr("To")

                TextField {
                    id: destinationField
                }
            }

            Button {
                id: goButton
                onClicked: getRoutes()
                text: qsTr("Compute route")
            }

            RoutePluginChooser {
                anchors.left: parent.left
                anchors.right: parent.right
                model: PluginManager.pluginModel("routing")
                onActivePluginChanged: root.plugin = PluginManager.loadPlugin(activePlugin)
            }
            Button {
                id: optionsButton
                text: qsTr("Options")
                visible: plugin != null && plugin.optionsUi != null
                onClicked: openOptions()
            }
        }

        BusyIndicator {
            id: busyIndicator
        }

        Loader {
            id: optionsLoader
        }
    }

    function getRoutes() {
        var model = plugin.routingModel()
        var origin = Mappero.isValid(originPoint) ? originPoint : currentPosition
        model.from = Mappero.point(origin)
        model.to = Mappero.point(destinationPoint)
        busyIndicator.running = true
        model.isRunningChanged.connect(onRouteReady)
        routeModel = model
        model.run()
    }

    function onRouteReady() {
        if (routeModel.running == false) {
            busyIndicator.running = false
            root.routesReady()
            console.log("route ready, count: " + routeModel.count)
        }
    }

    function openOptions() {
        optionsLoader.setSource("RouteOptions.qml", {
            "source": optionsButton,
            "optionsComponent": plugin.optionsUi
        })
        optionsLoader.item.open()
    }
}

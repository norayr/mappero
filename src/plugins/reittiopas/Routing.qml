import QtQuick 2.0
import Mappero 1.0
import Mappero.Plugin 1.0

RoutingModel {
    id: root

    wayPointDelegate: BaseWayPointDelegate {
        width: 0
        height: width
        transformOrigin: Item.Bottom

        Loader {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            sourceComponent: ("type" in model.data && model.data.subType != "walk") ? transportComponent: null
        }

        Component {
            id: transportComponent

            Image {
                source: "qrc:poi-arrow-down"
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                width: 32
                Rectangle {
                    anchors.bottom: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    radius: 4
                    color: "white"
                    width: childrenRect.width + 4
                    height: childrenRect.height + 4

                    Item {
                        id: transportLine
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: 2
                        width: childrenRect.width
                        height: childrenRect.height

                        Image {
                            id: image
                            anchors.left: parent.left
                            anchors.top: parent.top
                            width: Mappero.uiScale * 16
                            height: width
                            sourceSize: Qt.size(width, height)
                            source: "qrc:/transport/" + model.data.subType
                        }
                        Text {
                            id: lineLabel
                            anchors.left: image.right
                            anchors.margins: 2
                            anchors.verticalCenter: image.verticalCenter
                            text: model.data.line
                        }
                    }
                    Text {
                        id: timeLabel
                        anchors.top: transportLine.bottom
                        anchors.margins: 2
                        anchors.horizontalCenter: transportLine.horizontalCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: Qt.formatDateTime(model.time, "hh:mm")
                    }
                }
            }
        }
    }
    routeDelegate: BaseRouteDelegate {
        property real walkingLength: 0
        clip: true
        ListView {
            id: listView
            anchors.fill: parent
            orientation: ListView.Horizontal
            model: changes
            header: headerComponent
            delegate: Item {
                property bool isLast: index === listView.count - 1
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: height * 3 / 5

                Item {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: childrenRect.height
                    Rectangle {
                        id: topItem
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 1
                        anchors.rightMargin: isLast ? 1 : 0
                        height: childrenRect.height
                        color: "white"

                        Image {
                            id: image
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 4
                            anchors.rightMargin: 4
                            height: width
                            sourceSize: Qt.size(width, height)
                            source: "qrc:/transport/" + model.subType
                        }
                        Text {
                            id: transportText
                            anchors.horizontalCenter: image.horizontalCenter
                            anchors.top: image.bottom
                            horizontalAlignment: Text.AlignHCenter
                            text: model.subType != "walk" ? model.line : Mappero.formatLength(model.length)
                        }
                    }
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: topItem.bottom
                        anchors.topMargin: -2
                        color: "black"
                        height: 6
                        z: -1
                    }
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: topItem.bottom
                        anchors.topMargin: 2
                        anchors.leftMargin: 1
                        anchors.rightMargin: isLast ? 1 : 0
                        height: childrenRect.height
                        color: "white"
                        Text {
                            anchors.horizontalCenter: parent.left
                            horizontalAlignment: Text.AlignHCenter
                            text: Qt.formatDateTime(model.time, "hh:mm")
                        }
                        Text {
                            anchors.horizontalCenter: parent.right
                            horizontalAlignment: Text.AlignHCenter
                            visible: isLast
                            text: Qt.formatDateTime(pathItem.endTime, "hh:mm")
                        }
                    }
                }
            }
        }
        Component {
            id: headerComponent
            Item {
                id: header
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: height
                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: header.height / 3
                        fontSizeMode: Text.Fit
                        font.pointSize: 16
                        minimumPointSize: 3
                        text: qsTr("<b>%1</b> - <b>%2</b><br>(%3)").arg(Qt.formatDateTime(pathItem.startTime, "hh:mm")).arg(Qt.formatDateTime(pathItem.endTime, "hh:mm")).arg(Mappero.formatDuration(pathItem.endTime - pathItem.startTime))
                    }
                    Text {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        text: qsTr("Walking:")
                    }
                    Text {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        font.bold: true
                        text: Mappero.formatLength(walkingLength)
                    }
                }
            }
        }
        ListModel {
            id: changes
        }

        Component.onCompleted: {
            var wayPointModel = pathItem.wayPointModel
            var count = wayPointModel.count
            walkingLength = 0
            for (var i = 0; i < count; i++) {
                var p = wayPointModel.get(i, "data")
                if ("type" in p) {
                    console.log("Adding point " + p)
                    console.log("p.line:" + p.line)
                    console.log("p.type:" + p.type)
                    console.log("p.subType:" + p.subType)
                    p["time"] = wayPointModel.get(i, "time")
                    delete p["text"]
                    changes.append(p)
                    if (p.type === 0) {
                        walkingLength += p.length
                    }
                }
            }
        }
    }

    property var timeRegExp: new RegExp("(\\d{4})(\\d{2})(\\d{2})(\\d{2})(\\d{2})")
    property var lineRegExp: new RegExp("[ 0]*(.*) *")

    PathBuilder {
        id: builder
        source: "reittiopas"
    }

    function run() {
        findPath(from, to, options)
    }

    function findPath(from, to, options) {
        var url = "http://api.reittiopas.fi/hsl/prod/?request=route" +
        "&user=mappero&pass=tbmaitw&detail=full&epsg_in=4326&epsg_out=4326" +
        "&from=" + from.y + "," + from.x +
        "&to=" + to.y + "," + to.x
        if ("optimize" in options) {
            url += "&optimize=" + options.optimize
        }
        console.log("url: " + url)
        root.running = true
        var xhr = new XMLHttpRequest;
        xhr.open("GET", url);
        xhr.onreadystatechange = function() {
            if (xhr.readyState == XMLHttpRequest.DONE) {
                //console.log("Response: " + xhr.responseText)
                var routes = JSON.parse(xhr.responseText)
                for (var i_route in routes) {
                    builder.clear()
                    var route = routes[i_route][0]
                    for (var leg_i in route.legs) {
                        var leg = route.legs[leg_i]
                        for (var point_i in leg.locs) {
                            var wp = leg.locs[point_i]
                            var p = {
                                "time": parseTime(wp.arrTime),
                                "lat": wp.coord.y,
                                "lon": wp.coord.x,
                                "text": wp.name
                            }
                            if (point_i == 0) {
                                parseType(leg.type, leg.code, p)
                                p["length"] = leg.length
                            }
                            builder.addPoint(p)
                        }
                    }
                    appendPath(builder.path)
                }
                root.running = false
            }
        }
        xhr.send();
    }

    function parseTime(timeString) {
        return Date.parse(timeString.replace(timeRegExp, "$1 $2 $3 $4:$5")) / 1000
    }

    function parseLine(lineString) {
    }

    function parseType(typeString, lineString, p) {
        var type = ""
        var subType = ""
        var line = ""
        if (typeString == "walk") {
            type = 0
            subType = "walk"
        } else {
            type = 3
            switch (typeString) {
                case "1":
                case "3":
                case "4":
                case "5":
                case "8":
                case "21":
                case "22":
                case "23":
                case "24":
                case "25":
                case "36":
                case "39":
                    subType = "bus"; break
                case "2": subType = "tram"; break
                case "6": subType = "metro"; break
                case "7": subType = "ferry"; break
                case "12": subType = "train"; break
            }
            if (!subType) console.log("Can't determine subtype for type " + typeString)
        }

        if (lineString) {
            line = lineString.substring(1, 5).replace(lineRegExp, "$1")
            if (subType == "train") line = line.substring(1)
            else if (subType == "metro") line = ""
        }

        p["type"] = type
        p["subType"] = subType
        p["line"] = line
    }
}

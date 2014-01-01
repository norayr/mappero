import QtQuick 2.0
import Mappero 1.0
import Mappero.Plugin 1.0

RoutingModel {
    id: root

    wayPointDelegate: DefaultWayPointDelegate {}
    routeDelegate: DefaultRouteDelegate {}

    PathBuilder {
        id: builder
    }

    function run() {
        findPath(from, to)
    }

    function findPath(from, to) {
        var url = "http://maps.googleapis.com/maps/api/directions/json?" +
        "origin=" + from.x + "," + from.y +
        "&destination=" + to.x + "," + to.y +
        "&mode=" + "walking" +
        "&sensor=true&alternatives=true"
        console.log("url: " + url)
        root.running = true
        var xhr = new XMLHttpRequest;
        xhr.open("GET", url);
        xhr.onreadystatechange = function() {
            if (xhr.readyState == XMLHttpRequest.DONE) {
                //console.log("Response: " + xhr.responseText)
                var a = JSON.parse(xhr.responseText);
                var routes = a.routes
                for (var i_route in routes) {
                    builder.clear()
                    var steps = routes[i_route].legs[0].steps
                    var time = 0
                    for (var b in steps) {
                        var o = steps[b]
                        var p = {
                            "lat": o.start_location.lat,
                            "lon": o.start_location.lng,
                            "text": o.html_instructions,
                            "time": time
                        }
                        switch (o.maneuver) {
                            case "turn-right": p["dir"] = "right"; break
                            case "turn-left": p["dir"] = "left"; break
                            case "turn-slight-right": p["dir"] = "slight-right"; break
                            case "turn-slight-left": p["dir"] = "slight-left"; break
                            case "turn-sharp-right": p["dir"] = "sharp-right"; break
                            case "turn-sharp-left": p["dir"] = "sharp-left"; break
                        }

                        builder.addPoint(p)

                        time += o.duration.value

                        var points = decodeLine(o.polyline.points)
                        var l = points.length
                        for (var i = 0; i < l; i++) {
                            var p = {
                                "lat": points[i][0],
                                "lon": points[i][1]
                            }
                            if (i == l - 1) {
                                p["time"] = time
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

    function decodeLine(encoded) {
        var len = encoded.length;
        var index = 0;
        var array = [];
        var lat = 0;
        var lng = 0;

        while (index < len) {
            var b;
            var shift = 0;
            var result = 0;
            do {
                b = encoded.charCodeAt(index++) - 63;
                result |= (b & 0x1f) << shift;
                shift += 5;
            } while (b >= 0x20);
            var dlat = ((result & 1) ? ~(result >> 1) : (result >> 1));
            lat += dlat;

            shift = 0;
            result = 0;
            do {
                b = encoded.charCodeAt(index++) - 63;
                result |= (b & 0x1f) << shift;
                shift += 5;
            } while (b >= 0x20);
            var dlng = ((result & 1) ? ~(result >> 1) : (result >> 1));
            lng += dlng;

            array.push([lat * 1e-5, lng * 1e-5]);
        }

        return array;
    }
}

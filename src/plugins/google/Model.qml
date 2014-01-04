import QtQuick 2.0
import Mappero 1.0

PoiModel {
    id: listModel
    roles: ["name", "address"]
    property string apiKey: "AIzaSyDdPxlISfxPRpL4Z-X5es5DWNSS28z1OF0"

    Component.onCompleted: {
        console.log("Model loaded" + search.location)
        search.onQueryChanged.connect(doQuery)
    }

    function doQuery() {
        listModel.clear()
        var url = "https://maps.googleapis.com/maps/api/place/nearbysearch/json?key=" + apiKey +
        "&location=" + search.location.x + "," + search.location.y + "&keyword=" +
        search.query + "&sensor=true&rankby=distance"
        console.log("url: " + url)
        var xhr = new XMLHttpRequest;
        xhr.open("GET", url);
        xhr.onreadystatechange = function() {
            if (xhr.readyState == XMLHttpRequest.DONE) {
                console.log("Response: " + xhr.responseText)
                var a = JSON.parse(xhr.responseText);
                for (var b in a.results) {
                    var o = a.results[b]
                    var point = Mappero.geo(o.geometry.location.lat, o.geometry.location.lng)
                    listModel.append({
                        name: o.name,
                        address: o.vicinity,
                        icon: o.icon,
                        geoPoint: point
                    });
                }
            }
        }
        xhr.send();
    }
}

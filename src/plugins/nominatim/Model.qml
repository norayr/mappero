import QtQuick 2.0
import Mappero 1.0

SearchModel {
    id: listModel
    roles: ["name", "icon"]

    Component.onCompleted: {
        search.onQueryChanged.connect(doQuery)
    }

    function doQuery() {
        listModel.clear()
        var url = "http://nominatim.openstreetmap.org/search?format=json&q=" +
        search.query + "+" + search.location.x + "," + search.location.y +
        "&limit=" + search.itemsPerPage
        var xhr = new XMLHttpRequest;
        xhr.open("GET", url);
        xhr.onreadystatechange = function() {
            if (xhr.readyState == XMLHttpRequest.DONE) {
                console.log("Response: " + xhr.responseText)
                var a = JSON.parse(xhr.responseText);
                for (var b in a) {
                    var o = a[b]
                    var point = Mappero.geo(o.lat, o.lon)
                    listModel.append({
                        name: o.display_name,
                        icon: o.icon,
                        geoPoint: point
                    });
                }
            }
        }
        xhr.send();
    }
}

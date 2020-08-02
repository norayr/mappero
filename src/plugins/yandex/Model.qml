import QtQuick 2.0
import Mappero 1.0

PoiModel {
    id: listModel
    roles: ["name", "icon"]

    Component.onCompleted: {
        search.onQueryChanged.connect(doQuery)
    }

    function doQuery() {
        listModel.clear()
        var url = "https://search-maps.yandex.ru/v1/" +
            "?apikey=4897c636-93f3-4337-a63b-559dde445cd9" +
            "&text=" + encodeURIComponent(search.query) +
            "&ll=" + search.location.lon + "," + search.location.lat +
            "&results=" + search.itemsPerPage +
            "&lang=ru_RU"
        var xhr = new XMLHttpRequest;
        xhr.open("GET", url);
        xhr.onreadystatechange = function() {
            if (xhr.readyState == XMLHttpRequest.DONE) {
                console.log("Response: " + xhr.responseText)
                var response = JSON.parse(xhr.responseText);
                var features = response.features
                for (var i = 0; i < features.length; i++) {
                    var feature = features[i]
                    console.log("Found place: " + JSON.stringify(feature))
                    var coordinates = feature.geometry.coordinates
                    var point = Mappero.geo(coordinates[1], coordinates[0])
                    listModel.append({
                        name: feature.properties.name,
                        geoPoint: point,
                    });
                }
            }
        }
        xhr.send();
    }
}

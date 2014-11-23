import QtQuick 2.0

ListModel {
    id: root

    ListElement {
        uid: "OpenStreetMap I"
        name: "OpenStreetMap"
        type: "XYZ_INV"
        url: "http://tile.openstreetmap.org/%0d/%d/%d.png"
        format: "png"
        minZoom: 3
        maxZoom: 19
        notice: "© <a href=\"http://www.openstreetmap.org/copyright\">OpenStreetMap</a> contributors"
    }

    ListElement {
        uid: "GoogleVector"
        name: "Google Vector"
        type: "XYZ_INV"
        url: "http://mt.google.com/vt?z=%d&x=%d&y=%0d"
        format: "png"
        minZoom: 1
        maxZoom: 19
        notice: "Powered by Google"
    }

    ListElement {
        uid: "GoogleSatellite"
        name: "Google Satellite"
        type: "XYZ_INV"
        url: "http://khm.google.com/kh/v=150&z=%d&x=%d&y=%0d"
        format: "jpeg"
        minZoom: 3
        maxZoom: 15
        notice: "Powered by Google"
    }

    ListElement {
        uid: "GoogleCycling"
        name: "Google Cycling"
        type: "XYZ_INV"
        url: "http://mt.google.com/vt?lyrs=m@119,bike&z=%d&x=%d&y=%0d"
        format: "png"
        minZoom: 1
        maxZoom: 19
        notice: "Powered by Google"
    }

    ListElement {
        uid: "Virtual Earth"
        name: "Virtual Earth"
        type: "QUAD_ZERO"
        url: "http://r0.ortho.tiles.virtualearth.net/tiles/r%0s.jpeg?g=50"
        format: "jpeg"
        minZoom: 2
        maxZoom: 19
        notice: "Powered by Bing"
    }

    ListElement {
        uid: "Virtual Earth Satellite"
        name: "Virtual Earth Satellite"
        url: "http://r0.ortho.tiles.virtualearth.net/tiles/a%0s.jpeg?g=50"
        type: "QUAD_ZERO"
        format: "jpeg"
        minZoom: 2
        maxZoom: 19
        notice: "Powered by Bing"
    }

    ListElement {
        uid: "Virtual Earth Hybrid"
        name: "Virtual Earth Hybrid"
        url: "http://r0.ortho.tiles.virtualearth.net/tiles/h%0s.jpeg?g=50"
        type: "QUAD_ZERO"
        format: "jpeg"
        minZoom: 2
        maxZoom: 19
        notice: "Powered by Bing"
    }

    ListElement {
        uid: "YandexVector"
        name: "Yandex Vector"
        url: "http://vec.maps.yandex.net/tiles?l=map&v=2.14.0&x=%d&y=%d&z=%d"
        type: "YANDEX"
        format: "png"
        minZoom: 4
        maxZoom: 19
        notice: "Powered by Yandex"
    }

    ListElement {
        uid: "YandexSatellite"
        name: "Yandex Satellite"
        url: "http://sat.maps.yandex.net/tiles?l=sat&v=1.36.0&x=%d&y=%d&z=%d"
        type: "YANDEX"
        format: "png"
        minZoom: 2
        maxZoom: 19
        notice: "Powered by Yandex"
    }

    ListElement {
        uid: "HereVector"
        name: "HERE vector"
        url: "http://1.base.maps.cit.api.here.com/maptile/2.1/maptile/newest/normal.day/%d/%d/%d/256/png8?app_id=ZcYzFLXh2pUuEsIRrb6q&app_code=uwv-Hyp-kCyvl-nP2VFh3w"
        type: "XYZ_INV"
        format: "png"
        minZoom: 2
        maxZoom: 19
        notice: "Powered by HERE"
    }

    function find(uid) {
        var l = root.count
        for (var i = 0; i < l; i++) {
            if (root.get(i).uid == uid) return i
        }
        return 0
    }
}

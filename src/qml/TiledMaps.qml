import QtQuick 1.0

ListModel {
    ListElement {
        uid: "OpenStreetMap I"
        name: "OpenStreet"
        type: "XYZ_INV"
        url: "http://tile.openstreetmap.org/%0d/%d/%d.png"
        format: "png"
        minZoom: 3
        maxZoom: 19
    }
}

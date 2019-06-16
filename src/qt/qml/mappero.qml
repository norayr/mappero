import QtQuick 2.0
import Mappero 1.0

Rectangle {
    width: 800
    height: 480

    Loader {
        id: loader
        anchors.fill: parent
        focus: true
        source: firstPage
    }

    function closeRequest() {
        return (loader.item && loader.item.hasOwnProperty("closeRequest")) ?
            loader.item.closeRequest() : true
    }
}

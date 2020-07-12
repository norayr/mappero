import QtQuick 2.0
import "UIConstants.js" as UI

Item {
    id: root

    default property alias children: content.children
    property Item source
    property bool isOpen: state == "open"
    property var _sourcePos
    property var _destPos
    property string position: ""
    property int minimumWidth: 100
    property int minimumHeight: 60
    property bool _fullPage: pageOverlay.needsFullPage()

    signal closed()

    width: openPopup.width
    height: openPopup.height
    state: "closed"

    onHeightChanged: _computePositions()
    Component.onCompleted: _computePositions()

    states: [
        State {
            name: "closed"
            PropertyChanges {
                target: animatedPopup
                opacity: 0
                x: closedPopup.x
                y: closedPopup.y
                width: closedPopup.width
                height: closedPopup.height
            }
            PropertyChanges { target: openPopup; visible: false }
        },
        State {
            name: "open"
            PropertyChanges {
                target: animatedPopup
                opacity: 1
                x: openPopup.x
                y: openPopup.y
                width: openPopup.width
                height: openPopup.height
            }
            PropertyChanges { target: openPopup; visible: true }
        }
    ]

    transitions: [
        Transition {
            from: "closed"; to: "open"
            SequentialAnimation {
                PropertyAction {
                    target: animatedPopup; properties: "visible"; value: true
                }
                NumberAnimation {
                    target: animatedPopup
                    properties: "x,y,width,height,opacity"
                    easing.type: Easing.InOutQuad
                    duration: 300
                }
                PropertyAction {
                    target: animatedPopup; properties: "visible"; value: false
                }
                PropertyAction {
                    target: openPopup; properties: "visible"
                }
            }
        }
    ]

    Item {
        id: pageOverlay
        anchors.fill: parent
        parent: getRootItem()

        MouseArea {
            anchors.fill: parent
            enabled: root.isOpen
            onClicked: root.close()
        }

        Item {
            id: closedPopup
            x: _sourcePos.x
            y: _sourcePos.y
            width: source ? source.width : undefined
            height: source ? source.height : undefined
        }

        Item {
            id: openPopup
            x: _destPos.x
            y: _destPos.y
            anchors.centerIn: root._fullPage ? parent : undefined
            width: root._fullPage ? parent.width : root.minimumWidth + UI.ToolbarMargins * 2
            height: root._fullPage ? parent.height : root.minimumHeight + UI.ToolbarMargins * 2

            PaneBackground {}
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.AllButtons
                onWheel: {}
            }
            Item {
                id: content
                anchors.fill: parent
                anchors.margins: UI.ToolbarMargins
            }
        }

        Item {
            id: animatedPopup
            visible: false
            PaneBackground {}
        }

        function needsFullPage() {
            return (root.minimumWidth >= parent.width * 2 / 3 ||
                    root.minimumHeight >= parent.height * 2 / 3);
        }
    }

    function getRootItem() {
        if (!source) return null;
        var rootItem = source
        while (rootItem.parent && !rootItem.hasOwnProperty("__isPage"))
        {
            rootItem = rootItem.parent
        }
        return rootItem;
    }

    function _computePositions() {
        _sourcePos = pageOverlay.mapFromItem(source, 0, 0)
        if (position == "") {
            _destPos = pageOverlay.mapFromItem(root, 0, 0)
        } else if (position == "pageCenter") {
            _destPos = Qt.point((pageOverlay.width - openPopup.width) / 2,
                                (pageOverlay.height - openPopup.height) / 2)
        } else {
            var dx = _sourcePos.x
            var dy = _sourcePos.y
            if (position == "left") dx -= openPopup.width + 20
            else if (position == "right") dx += source.width + 20
            else if (position == "bottom") dy += source.height + 20
            else if (position == "top") dy -= openPopup.height + 20
            _destPos = Qt.point(dx, dy)
        }
        console.log("Destpos: " + _destPos.x + "," + _destPos.y)
    }

    function _computeZ() {
        var z = 0
        var children = pageOverlay.parent.children
        for (var i = 0; i < children.length; i++) {
            z = Math.max(z, children[i].z)
        }
        return z + 1
    }

    function open() {
        pageOverlay.z = _computeZ()
        _computePositions()
        state = "open"
    }

    function close() {
        _computePositions()
        state = "closed"
        closed()
    }
}

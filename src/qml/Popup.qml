import QtQuick 1.0
import "UIConstants.js" as UI

Item {
    id: root

    default property alias children: content.children
    property Item source
    property bool isOpen: state == "open"
    property variant _sourcePos

    parent: getRootItem()
    state: "closed"

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
        id: closedPopup
        x: _sourcePos.x
        y: _sourcePos.y
        width: source.width
        height: source.height
    }

    Item {
        id: openPopup
        // FIXME: check whether the item fits in this position
        x: _sourcePos.x - width - 20
        y: _sourcePos.y
        width: content.childrenRect.width + UI.ToolbarMargins * 2
        height: content.childrenRect.height + UI.ToolbarMargins * 2

        PaneBackground {}
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

    function getRootItem() {
        var rootItem = source.parent
        while (rootItem.parent && !rootItem.hasOwnProperty("__isPage"))
        {
            rootItem = rootItem.parent
        }
        return rootItem;
    }
    function _computePositions() {
        _sourcePos = parent.mapFromItem(source, 0, 0)
    }

    function open() {
        _computePositions()
        state = "open"
    }

    function close() {
        _computePositions()
        state = "closed"
    }
}

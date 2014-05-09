import Mappero 1.0
import QtQuick 2.0

Selector {
    id: root

    property string textRole: "text"
    property string iconRole: ""

    buttonDelegate: ButtonWithIcon {
        text: model[root.textRole]
        iconSource: root.iconRole ? model[root.iconRole] : ""
    }
    listDelegate: ButtonWithIcon {
        text: model[root.textRole]
        iconSource: root.iconRole ? model[root.iconRole] : ""
        onClicked: ListView.view.selectItem(index)
    }
}

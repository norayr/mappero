import QtQuick 2.0
import Mappero.Ui 1.0

SimpleSelector {
    id: root

    property string activePlugin: model.get(selectedIndex, "name")

    textRole: "displayName"
    iconRole: "icon"
}

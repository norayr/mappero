import QtQuick 1.0
import com.nokia.meego 1.0

SelectionDialogStyle {
    property int itemHeight: 64
    property color itemTextColor: inverted ? "#fff" : "#000"
    property color itemSelectedTextColor: inverted ? "#fff" : "#000"

    // Background
    property string itemBackground: ""
    property color itemBackgroundColor: "transparent"
    property color itemSelectedBackgroundColor: "#3D3D3D"
    property string itemSelectedBackground: "" // "image://theme/meegotouch-list-fullwidth-background-selected"
    property string itemPressedBackground: "image://theme/meegotouch-panel" + __invertedString + "-background-pressed"
}

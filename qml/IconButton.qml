import QtQuick
import QtQuick.Controls as Controls

Controls.ToolButton {
    id: button
    required property string label
    required property string iconKey
    property string themeName: ""
    implicitWidth: 36
    implicitHeight: 36
    display: Controls.AbstractButton.IconOnly
    icon.name: themeName
    icon.source: "qrc:/assets/" + iconKey + ".svg"
    icon.width: 20
    icon.height: 20
    Accessible.name: label
    Controls.ToolTip.text: label
    Controls.ToolTip.visible: hovered
    Controls.ToolTip.delay: 500
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: button.palette.highlight
        border.width: button.visualFocus ? 2 : 0
        radius: 4
    }
}

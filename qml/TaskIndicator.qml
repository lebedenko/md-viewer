pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QMarkdown 1.14

Item {
    id: adapter
    required property bool checked
    required property MarkdownStyle markdownStyle
    property alias hostPalette: control.palette
    implicitWidth: control.indicator ? control.indicator.implicitWidth : 0
    implicitHeight: control.indicator ? control.indicator.implicitHeight : 0
    Accessible.ignored: true

    Controls.CheckBox {
        id: control
        objectName: "taskArtworkControl"
        visible: false
        enabled: true
        focusPolicy: Qt.NoFocus
        checked: adapter.checked
        font: adapter.markdownStyle.bodyFont
        Accessible.ignored: true
    }
    Binding { target: control.indicator; property: "parent"; value: adapter }
    Binding { target: control.indicator; property: "x"; value: 0 }
    Binding { target: control.indicator; property: "y"; value: 0 }
    Binding { target: control.indicator; property: "visible"; value: true }
}

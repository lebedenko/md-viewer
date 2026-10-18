import QtQuick
import QtQuick.Controls as Controls

Item {
    id: root
    required property string languageName
    required property string codeText
    required property var requestCopy
    implicitWidth: copyButton.implicitWidth + (languageLabel.visible ? languageLabel.implicitWidth + 8 : 0)
    implicitHeight: Math.max(copyButton.implicitHeight, languageLabel.visible ? languageLabel.implicitHeight : 0)
    Controls.ToolButton {
        id: copyButton
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: Math.min(implicitWidth, parent.width)
        objectName: "codeCopyButton"
        text: qsTr("Copy code")
        display: Controls.AbstractButton.IconOnly
        icon.source: "qrc:/assets/copy.svg"
        icon.width: 16
        icon.height: 16
        icon.color: palette.buttonText
        padding: 4
        Accessible.name: text
        Controls.ToolTip.visible: hovered
        Controls.ToolTip.text: text
        onClicked: root.requestCopy()
    }
    Controls.Label {
        id: languageLabel
        anchors.left: parent.left
        width: Math.max(0, copyButton.x - 8)
        elide: Text.ElideRight
        objectName: "codeLanguageLabel"
        text: root.languageName
        visible: text.length > 0
        anchors.verticalCenter: parent.verticalCenter
    }
}

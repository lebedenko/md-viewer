import QtQuick
import QtQuick.Controls as Controls

Row {
    id: root
    required property string languageName
    required property string codeText
    required property var requestCopy
    spacing: 8
    Controls.Label { text: root.languageName; anchors.verticalCenter: parent.verticalCenter }
    Controls.ToolButton { objectName: "codeCopyButton"; text: qsTr("Copy code"); onClicked: root.requestCopy() }
}

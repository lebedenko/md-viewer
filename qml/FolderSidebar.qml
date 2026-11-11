pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import MdViewer

Rectangle {
    id: root
    required property FolderModel folder
    property color textColor: "#202020"
    property color accentColor: "#0066cc"
    readonly property color separatorColor: Qt.alpha(textColor, 0.2)
    signal fileActivated(url file)

    function focusTree() {
        if (list.count > 0)
            list.focusRow(Math.max(0, root.folder.currentRow));
    }

    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.rightMargin: 1
        spacing: 0

        Controls.Label {
            objectName: "folderName"
            Layout.fillWidth: true
            Layout.margins: 12
            Layout.bottomMargin: 8
            text: root.folder.name
            textFormat: Text.PlainText
            color: root.textColor
            font.bold: true
            elide: Text.ElideRight
            Controls.ToolTip.text: root.folder.displayPath
            Controls.ToolTip.visible: nameHover.hovered
            Controls.ToolTip.delay: 500
            HoverHandler { id: nameHover }
        }
        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Controls.ScrollBar.horizontal.policy: Controls.ScrollBar.AlwaysOff
            ListView {
                id: list
                objectName: "folderTree"
                property real savedY: 0
                property bool savedFocus: false
                model: root.folder
                boundsBehavior: Flickable.StopAtBounds
                currentIndex: -1
                bottomMargin: 12
                clip: true

                function focusRow(index) {
                    if (index < 0 || index >= count)
                        return;
                    currentIndex = index;
                    positionViewAtIndex(index, ListView.Contain);
                    const row = itemAtIndex(index);
                    if (row)
                        row.forceActiveFocus(Qt.TabFocusReason);
                }

                // A rescan resets the model; keep the reading position and keyboard focus.
                Connections {
                    target: root.folder
                    function onModelAboutToBeReset() {
                        list.savedY = list.contentY;
                        list.savedFocus = list.activeFocus;
                    }
                    function onModelReset() {
                        list.contentY = Math.max(0, Math.min(list.savedY, list.contentHeight - list.height));
                        if (list.savedFocus)
                            Qt.callLater(root.focusTree);
                    }
                    function onCurrentRowChanged() {
                        if (root.folder.currentRow >= 0)
                            list.positionViewAtIndex(root.folder.currentRow, ListView.Contain);
                    }
                }

                delegate: Controls.ItemDelegate {
                    id: row
                    required property string entryName
                    required property url entryUrl
                    required property bool isDirectory
                    required property int depth
                    required property bool expanded
                    required property int index
                    readonly property bool isCurrent: root.folder.currentRow === index
                    readonly property bool tabStop: list.currentIndex === index || (list.currentIndex < 0 && index === 0)

                    width: list.width
                    leftPadding: 12 + depth * 14
                    rightPadding: 8
                    topPadding: 5
                    bottomPadding: 5
                    focusPolicy: tabStop ? Qt.StrongFocus : Qt.ClickFocus
                    highlighted: activeFocus
                    contentItem: RowLayout {
                        spacing: 4
                        Text {
                            Layout.preferredWidth: 12
                            text: row.isDirectory ? (row.expanded ? "▾" : "▸") : ""
                            color: root.textColor
                            opacity: 0.7
                            font: row.font
                        }
                        Text {
                            Layout.fillWidth: true
                            text: row.entryName
                            textFormat: Text.PlainText
                            elide: Text.ElideRight
                            color: row.isCurrent ? root.accentColor : root.textColor
                            font.family: row.font.family
                            font.pointSize: row.font.pointSize
                            font.bold: row.isCurrent
                        }
                    }
                    background: Rectangle {
                        color: row.hovered || row.activeFocus ? Qt.alpha(root.textColor, 0.07) : "transparent"
                        border.width: row.visualFocus ? 1 : 0
                        border.color: root.accentColor
                        Rectangle {
                            width: 3
                            height: parent.height
                            visible: row.isCurrent
                            color: root.accentColor
                        }
                    }
                    Controls.ToolTip.text: entryName
                    Controls.ToolTip.visible: hovered
                    Controls.ToolTip.delay: 800
                    Accessible.name: entryName
                    Accessible.role: Accessible.TreeItem
                    onActiveFocusChanged: if (activeFocus) list.currentIndex = index
                    onClicked: {
                        list.currentIndex = index;
                        if (isDirectory)
                            root.folder.toggle(index);
                        else
                            root.fileActivated(entryUrl);
                    }
                    Keys.onDownPressed: list.focusRow(index + 1)
                    Keys.onUpPressed: list.focusRow(index - 1)
                    Keys.onRightPressed: {
                        if (isDirectory && !expanded)
                            root.folder.toggle(index);
                        else if (isDirectory)
                            list.focusRow(index + 1);
                    }
                    Keys.onLeftPressed: {
                        if (isDirectory && expanded)
                            root.folder.toggle(index);
                        else
                            list.focusRow(root.folder.parentRow(index));
                    }
                    Keys.onReturnPressed: clicked()
                    Keys.onEnterPressed: clicked()
                }
            }
        }
        Controls.Label {
            objectName: "folderNote"
            Layout.fillWidth: true
            Layout.margins: 12
            visible: text !== ""
            text: root.folder.truncated ? qsTr("Listing is incomplete: more than %1 Markdown files.").arg(root.folder.fileCount)
                : root.folder.fileCount === 0 ? qsTr("No Markdown files in this folder.") : ""
            color: root.textColor
            opacity: 0.7
            wrapMode: Text.WordWrap
        }
    }
    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: root.separatorColor
    }
}

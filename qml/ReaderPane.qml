import QtQuick
import QtQuick.Controls as Controls
import QMarkdown 1.10

Rectangle {
    id: root
    property alias markdown: document.markdown
    property bool dark: false
    property color paperColor: "white"
    property color textColor: "#202020"
    property color linkColor: "#0066cc"
    property color selectionColor: "#803b82f6"
    readonly property color separatorColor: Qt.alpha(textColor, 0.2)
    readonly property color codeBackgroundColor: Qt.alpha(textColor, 0.07)
    signal copyRequested(string text)
    color: paperColor

    Flickable {
        id: viewport
        anchors.fill: parent
        contentWidth: width
        contentHeight: document.height + 48
        clip: true
        Controls.ScrollBar.vertical: Controls.ScrollBar {}

        MarkdownView {
            id: document
            objectName: "markdownDocument"
            x: (viewport.width - width) / 2
            y: 24
            width: Math.max(0, Math.min(800, viewport.width - 48))
            selectionEnabled: true
            gfmEnabled: true
            style: MarkdownStyle {
                preset: root.dark ? MarkdownStyle.PolishedDark : MarkdownStyle.PolishedLight
                bodyColor: root.textColor
                h1Color: root.textColor
                h2Color: root.textColor
                h3Color: root.textColor
                h4Color: root.textColor
                h5Color: root.textColor
                h6Color: root.textColor
                codeBlockColor: root.textColor
                linkColor: root.linkColor
                selectionBackgroundColor: root.selectionColor
                headings.h1.separatorColor: root.separatorColor
                headings.h2.separatorColor: root.separatorColor
                codeBlock.backgroundColor: root.codeBackgroundColor
                codeBlock.borderColor: root.separatorColor
                inlineCode.backgroundColor: root.codeBackgroundColor
                table.borderColor: root.separatorColor
            }
            onCopyRequested: function(text) { root.copyRequested(text) }
            onCodeBlockCopyRequested: function(text) { root.copyRequested(text) }
            codeBlockActions: Component {
                Controls.ToolButton {
                    required property string codeText
                    required property var requestCopy
                    text: qsTr("Copy code")
                    onClicked: requestCopy()
                }
            }
        }
    }
}

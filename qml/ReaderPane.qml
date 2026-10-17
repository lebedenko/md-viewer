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
    property real zoom: 1.0
    property string bodyFamily: ""
    property string codeFamily: ""
    property real blockSpacingScale: 1.0
    property bool linkUnderline: true
    property real maxColumnWidth: 800
    signal copyRequested(string text)

    // The renderer has no zoom API: scale the preset's own point sizes through public style properties.
    MarkdownStyle {
        id: baseline
        preset: root.dark ? MarkdownStyle.PolishedDark : MarkdownStyle.PolishedLight
    }

    function scaledFont(base, family) {
        const scaled = Qt.font({
            "family": family || base.family, "weight": base.weight, "italic": base.italic,
            "pointSize": base.pointSize * root.zoom
        })
        return scaled
    }

    function resetScroll() {
        viewport.contentY = 0
    }

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
            width: Math.max(0, Math.min(root.maxColumnWidth, viewport.width - 48))
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
                bodyFont: root.scaledFont(baseline.bodyFont, root.bodyFamily)
                codeBlockFont: root.scaledFont(baseline.codeBlockFont, root.codeFamily)
                inlineCodeFont: root.scaledFont(baseline.inlineCodeFont, root.codeFamily)
                h1Font: root.scaledFont(baseline.h1Font, root.bodyFamily)
                h2Font: root.scaledFont(baseline.h2Font, root.bodyFamily)
                h3Font: root.scaledFont(baseline.h3Font, root.bodyFamily)
                h4Font: root.scaledFont(baseline.h4Font, root.bodyFamily)
                h5Font: root.scaledFont(baseline.h5Font, root.bodyFamily)
                h6Font: root.scaledFont(baseline.h6Font, root.bodyFamily)
                linkColor: root.linkColor
                linkUnderline: root.linkUnderline
                blockSpacing: baseline.blockSpacing * root.blockSpacingScale
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

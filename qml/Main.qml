import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import MdViewer

Controls.ApplicationWindow {
    id: window
    readonly property string documentTitle: qsTr("Markdown Viewer")
    readonly property string documentSource: qsTr("Bundled sample")

    width: 960
    height: 720
    visible: true
    title: documentTitle

    header: Controls.ToolBar {
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 0

            Controls.Label {
                Layout.fillWidth: true
                text: window.documentTitle
                font.bold: true
                elide: Text.ElideRight
            }
            Controls.Label {
                Layout.fillWidth: true
                text: window.documentSource
                opacity: 0.7
                font.pointSize: window.font.pointSize * 0.85
                elide: Text.ElideMiddle
            }
        }
    }

    footer: Controls.ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            Controls.Label {
                text: window.documentSource
                opacity: 0.7
            }
            Item { Layout.fillWidth: true }
            Controls.Label {
                text: qsTr("Markdown · GFM")
                opacity: 0.7
            }
        }
    }

    ReaderPane {
        anchors.fill: parent
        dark: window.palette.base.hslLightness < 0.5
        paperColor: window.palette.base
        textColor: window.palette.text
        linkColor: window.palette.link
        selectionColor: window.palette.highlight
        markdown: "# Markdown Viewer\n\nA comfortable place to read local Markdown documents.\n\n## Reading\n\nSelect text to copy it, or try the copy action on this code block.\n\n```sh\necho 'Hello, Markdown'\n```\n\n## GitHub Flavored Markdown\n\n| Feature | Status |\n| :--- | ---: |\n| Tables | ready |\n| Task lists | ready |\n\n- [x] Strikethrough: ~~old~~ new\n- [ ] Open files\n\nAutolink: https://www.qt.io\n\nFile opening and document navigation are coming next."
        onCopyRequested: function(text) { Clipboard.copyText(text) }
    }
}

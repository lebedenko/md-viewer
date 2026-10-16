import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import MdViewer

Controls.ApplicationWindow {
    id: window
    property string initialPath
    readonly property string documentTitle: session.title || qsTr("Markdown Viewer")
    readonly property string documentSource: session.revision === 0 ? qsTr("Bundled sample") : session.displayPath
    readonly property string statusMessage: session.error ? "\u26A0 " + session.error
        : (session.warning ? "\u26A0 " + session.warning : "")
    readonly property string sampleMarkdown: "# Markdown Viewer\n\nA comfortable place to read local Markdown documents.\n\n## Reading\n\nSelect text to copy it, or try the copy action on this code block.\n\n```sh\necho 'Hello, Markdown'\n```\n\n## GitHub Flavored Markdown\n\n| Feature | Status |\n| :--- | ---: |\n| Tables | ready |\n| Task lists | ready |\n\n- [x] Strikethrough: ~~old~~ new\n- [x] Open files\n\nAutolink: https://www.qt.io\n\nOpen a file with Ctrl+O, drop one onto this window, or pass a path on the command line."

    width: 960
    height: 720
    visible: true
    title: documentTitle

    DocumentSession {
        id: session
    }

    Component.onCompleted: {
        if (initialPath)
            session.openPath(initialPath)
    }

    Shortcut {
        sequences: [StandardKey.Open]
        onActivated: openDialog.open()
    }

    FileDialog {
        id: openDialog
        title: qsTr("Open Markdown file")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Markdown files (*.md *.markdown)"), qsTr("All files (*)")]
        onAccepted: session.open(selectedFile)
    }

    header: Controls.ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 8
            spacing: 8

            ColumnLayout {
                Layout.fillWidth: true
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
            Controls.ToolButton {
                text: qsTr("Open…")
                onClicked: openDialog.open()
            }
        }
    }

    footer: Controls.ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            Controls.Label {
                Layout.fillWidth: true
                text: window.statusMessage || window.documentSource
                opacity: window.statusMessage ? 1 : 0.7
                font.bold: window.statusMessage !== ""
                elide: Text.ElideMiddle
            }
            Controls.Label {
                text: qsTr("Markdown · GFM")
                opacity: 0.7
            }
        }
    }

    ReaderPane {
        id: reader
        anchors.fill: parent
        dark: window.palette.base.hslLightness < 0.5
        paperColor: window.palette.base
        textColor: window.palette.text
        linkColor: window.palette.link
        selectionColor: window.palette.highlight
        markdown: session.revision === 0 ? window.sampleMarkdown : session.markdown
        onCopyRequested: function(text) { Clipboard.copyText(text) }
    }

    Connections {
        target: session
        function onDocumentChanged() {
            reader.resetScroll()
        }
    }

    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]
        onEntered: function(drag) {
            drag.accepted = drag.hasUrls && drag.urls.length > 0
                && (drag.supportedActions & Qt.CopyAction) !== 0
        }
        onDropped: function(drop) {
            if (!drop.hasUrls || drop.urls.length === 0 || (drop.supportedActions & Qt.CopyAction) === 0)
                return
            const url = drop.urls[0]
            session.open(url)
            if (url.toString().toLowerCase().startsWith("file:"))
                drop.accept(Qt.CopyAction)
        }

        Rectangle {
            anchors.fill: parent
            visible: parent.containsDrag
            color: Qt.alpha(window.palette.highlight, 0.15)
            border.width: 3
            border.color: window.palette.highlight
        }
    }
}

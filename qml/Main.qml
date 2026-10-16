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

    readonly property var zoomSteps: [0.5, 0.67, 0.8, 0.9, 1.0, 1.1, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0]

    function stepZoom(direction) {
        let index = 0
        for (let i = 0; i < zoomSteps.length; ++i) {
            if (Math.abs(zoomSteps[i] - config.zoom) < Math.abs(zoomSteps[index] - config.zoom))
                index = i
        }
        config.zoom = zoomSteps[Math.max(0, Math.min(zoomSteps.length - 1, index + direction))]
    }

    readonly property var widthPresets: [
        { "name": qsTr("Narrow"), "width": 600 },
        { "name": qsTr("Comfortable"), "width": 800 },
        { "name": qsTr("Wide"), "width": 1000 }
    ]

    function cycleWidth() {
        config.widthPreset = (Math.max(0, Math.min(widthPresets.length - 1, config.widthPreset)) + 1)
            % widthPresets.length
    }

    AppConfig {
        id: config
    }

    Shortcut {
        sequences: ["Ctrl+Shift+W"]
        onActivated: window.cycleWidth()
    }

    Shortcut {
        sequences: [StandardKey.ZoomIn, "Ctrl+="]
        onActivated: window.stepZoom(1)
    }

    Shortcut {
        sequences: [StandardKey.ZoomOut]
        onActivated: window.stepZoom(-1)
    }

    Shortcut {
        sequences: ["Ctrl+0"]
        onActivated: config.zoom = 1.0
    }

    Shortcut {
        sequences: [StandardKey.Open]
        onActivated: openDialog.open()
    }

    Shortcut {
        sequences: [StandardKey.Preferences, "Ctrl+,"]
        onActivated: settingsDialog.open()
    }

    SettingsDialog {
        id: settingsDialog
        objectName: "settingsDialog"
        appConfig: config
        widthPresets: window.widthPresets
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
            Controls.ToolButton {
                objectName: "settingsButton"
                text: qsTr("Settings")
                Controls.ToolTip.text: qsTr("Settings (Ctrl+,)")
                Controls.ToolTip.visible: hovered
                onClicked: settingsDialog.open()
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
        objectName: "readerPane"
        anchors.fill: parent
        dark: window.palette.base.hslLightness < 0.5
        paperColor: window.palette.base
        textColor: window.palette.text
        linkColor: window.palette.link
        selectionColor: window.palette.highlight
        zoom: config.zoom
        maxColumnWidth: window.widthPresets[Math.max(0, Math.min(window.widthPresets.length - 1,
                                                                  config.widthPreset))].width
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

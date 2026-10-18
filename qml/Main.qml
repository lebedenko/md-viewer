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
    readonly property string statusMessage: navigation.error ? "⚠ " + navigation.error : session.error ? "\u26A0 " + session.error
        : (session.warning ? "\u26A0 " + session.warning : "")
    readonly property string sampleMarkdown: "# Markdown Viewer\n\nA comfortable place to read local Markdown documents.\n\n## Reading\n\nSelect text to copy it, or try the copy action on this code block.\n\n```sh\necho 'Hello, Markdown'\n```\n\n## GitHub Flavored Markdown\n\n| Feature | Status |\n| :--- | ---: |\n| Tables | ready |\n| Task lists | ready |\n\n- [x] Strikethrough: ~~old~~ new\n- [x] Open files\n\nAutolink: https://www.qt.io\n\nOpen a file with Ctrl+O, drop one onto this window, or pass a path on the command line."

    width: 960
    height: 720
    visible: true
    title: documentTitle

    readonly property DocumentSession session: navigation.session
    NavigationController { id: navigation }

    Component.onCompleted: {
        if (initialPath)
            navigation.openPath(initialPath)
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
        { "value": AppConfig.Narrow, "name": qsTr("Narrow"), "width": 600 },
        { "value": AppConfig.Comfortable, "name": qsTr("Comfortable"), "width": 800 },
        { "value": AppConfig.Wide, "name": qsTr("Wide"), "width": 1000 }
    ]

    readonly property var themes: [
        { "value": AppConfig.System, "name": qsTr("System") },
        { "value": AppConfig.Light, "name": qsTr("Light") },
        { "value": AppConfig.Dark, "name": qsTr("Dark") }
    ]

    readonly property int widthPresetIndex: Math.max(0, widthPresets.findIndex(preset => preset.value === config.widthPreset))

    function cycleWidth() {
        config.widthPreset = widthPresets[(widthPresetIndex + 1) % widthPresets.length].value
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

    Shortcut { sequence: "Ctrl+F"; onActivated: reader.openFind() }
    Shortcut { sequence: "Ctrl+Shift+O"; onActivated: reader.outlineVisible = !reader.outlineVisible }
    Shortcut { sequence: "Alt+Left"; onActivated: navigation.back() }
    Shortcut { sequence: "Alt+Right"; onActivated: navigation.forward() }
    Shortcut { sequence: "Ctrl+A"; onActivated: { config.selectionEnabled = true; reader.selectAll() } }
    SettingsDialog {
        id: settingsDialog
        objectName: "settingsDialog"
        appConfig: config
        widthPresets: window.widthPresets
        themes: window.themes
        mathAvailable: reader.mathAvailable
        highlightingAvailable: reader.highlightingAvailable
    }

    FileDialog {
        id: openDialog
        title: qsTr("Open Markdown file")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Markdown files (*.md *.markdown)"), qsTr("All files (*)")]
        onAccepted: navigation.open(selectedFile)
    }

    header: Controls.ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 8
            spacing: 8

            Controls.ToolButton { text: qsTr("Back"); enabled: navigation.canGoBack; onClicked: navigation.back() }
            Controls.ToolButton { text: qsTr("Forward"); enabled: navigation.canGoForward; onClicked: navigation.forward() }
            Controls.ToolButton { text: qsTr("Outline"); checkable: true; checked: reader.outlineVisible; onClicked: reader.outlineVisible = checked }
            Controls.ToolButton { text: qsTr("Find"); onClicked: reader.openFind() }
            Controls.CheckBox {
                text: qsTr("Select text")
                checked: config.selectionEnabled
                onToggled: config.selectionEnabled = checked
                Controls.ToolTip.text: qsTr("Selection displays math as TeX source")
                Controls.ToolTip.visible: hovered
            }
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
        dark: config.theme === AppConfig.System ? window.palette.base.hslLightness < 0.5 : config.theme === AppConfig.Dark
        paperColor: config.theme === AppConfig.System ? window.palette.base : dark ? "#1e1f22" : "#ffffff"
        textColor: config.theme === AppConfig.System ? window.palette.text : dark ? "#e3e5e8" : "#1f2328"
        linkColor: config.theme === AppConfig.System ? window.palette.link : dark ? "#6cb0ff" : "#0969da"
        selectionColor: config.theme === AppConfig.System ? window.palette.highlight : dark ? "#803b82f6" : "#80338bff"
        zoom: config.zoom
        bodyFamily: config.bodyFont
        codeFamily: config.codeFont
        blockSpacingScale: config.blockSpacing
        linkUnderline: config.linkUnderline
        maxColumnWidth: window.widthPresets[window.widthPresetIndex].width
        markdown: window.session.revision === 0 ? window.sampleMarkdown : window.session.markdown
        selectionEnabled: config.selectionEnabled
        mathEnabled: config.mathEnabled
        highlightingEnabled: config.highlightingEnabled
        codeWrapping: config.codeWrapping
        lineNumbers: config.lineNumbers
        heightLimiting: config.heightLimiting
        baseUrl: window.session.url
        imageRoots: window.session.revision > 0 ? [navigation.imageRoot] : []
        onPositionChanged: fraction => navigation.setPosition(fraction)
        onLinkActivated: destination => navigation.activate(destination)
        onMissingFragment: fragment => navigation.missingFragment(fragment)
        onCopyRequested: function(text) { Clipboard.copyText(text) }
    }

    Connections {
        target: window.session
        function onStateChanged() {
            if (window.session.state === DocumentSession.Loading) {
                ++window.restorationIntent
                reader.restoring = true
            } else if (window.session.state === DocumentSession.Failed) {
                reader.restoring = false
            }
        }
    }
    property int restorationIntent: 0
    Connections {
        target: navigation
        function onNavigating() { ++window.restorationIntent; reader.restoring = false; reader.closeFind() }
        function onRestoreRequested(fraction, fragment, reload, newDocument) {
            const intent = ++window.restorationIntent
            if (!newDocument) {
                reader.restore(fraction, fragment, reload, false)
                return
            }
            reader.restoring = true
            reader.navigationAllowed = true
            Qt.callLater(function() {
                if (intent === window.restorationIntent && reader.navigationAllowed)
                    reader.restore(fraction, fragment, reload, true)
            })
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
            navigation.open(url)
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

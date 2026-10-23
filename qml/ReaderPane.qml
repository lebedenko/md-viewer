pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QMarkdown 1.17

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
    property bool selectionEnabled: true
    property bool mathEnabled: true
    property bool highlightingEnabled: true
    property bool codeWrapping: false
    property bool lineNumbers: true
    property bool heightLimiting: true
    property url baseUrl
    property var imageRoots: []
    property bool outlineVisible: true
    property int activatedHeading: -1
    onMarkdownChanged: { activatedHeading = -1; outlineTarget = -1 }
    property bool findVisible: false
    readonly property bool mathAvailable: document.mathAvailable
    readonly property bool highlightingAvailable: document.syntaxHighlightingAvailable
    signal linkActivated(string destination)
    signal positionChanged(real fraction)
    property int outlineTarget: -1
    property bool navigationAllowed: true
    function manualScroll() { navigationAllowed = false; restoring = false; restoreTimer.stop() }
    function nextResult(previous) {
        navigationAllowed = true
        if (previous) document.searchPrevious(); else document.searchNext()
    }
    Keys.onShortcutOverride: event => {
        if (event.key === Qt.Key_Tab || event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
            navigationAllowed = true
        event.accepted = false
    }
    function openFind() { findVisible = true; query.forceActiveFocus(); query.selectAll() }
    function closeFind() { findVisible = false; document.forceActiveFocus() }
    function selectAll() { document.selectAll(); document.forceActiveFocus() }
    function restore(fraction, fragment, reload, newDocument) {
        restoreTimer.stop()
        restoring = false
        navigationAllowed = true
        if (reload && findVisible && query.text.length) {
            document.searchActive = false
            document.searchActive = Qt.binding(() => root.findVisible)
        } else if (fragment.length) {
            const heading = newDocument ? -1 : outlineTarget
            outlineTarget = -1
            const found = heading >= 0 ? document.navigateToHeading(heading) : document.navigateToFragment("#" + fragment)
            if (found && heading >= 0) activatedHeading = heading
            if (!found) {
                if (newDocument) resetScroll()
                missingFragment(fragment)
            }
        } else { savedFraction = fraction; restorePosition() }
    }
    signal missingFragment(string fragment)
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
        restoring = false
        viewport.contentY = 0
    }

    // Reading position survives a reload as a fraction of the scrollable height.
    property real savedFraction: 0
    property bool restoring: false

    function restorePosition() {
        restoring = true
        applyPosition()
        restoreTimer.restart()
    }

    function applyPosition() {
        const range = viewport.contentHeight - viewport.height
        viewport.contentY = range > 0 ? savedFraction * range : 0
    }

    Timer {
        id: restoreTimer
        interval: 400
        onTriggered: root.restoring = false
    }

    color: paperColor

    ColumnLayout {
        id: searchRow
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        visible: root.findVisible
        height: visible ? implicitHeight : 0
        RowLayout {
            Layout.fillWidth: true
            Controls.TextField {
                id: query
                Layout.fillWidth: true
                onTextChanged: root.navigationAllowed = true
                placeholderText: qsTr("Find in document")
                onAccepted: root.nextResult(false)
                Keys.onEscapePressed: root.closeFind()
                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Return && (event.modifiers & Qt.ShiftModifier)) {
                        root.nextResult(true); event.accepted = true
                    }
                }
            }
            Controls.CheckBox { id: matchCase; text: qsTr("Match case"); onToggled: root.navigationAllowed = true }
            Controls.Label { text: (document.currentSearchResult + 1) + " / " + document.searchResultCount }
            Controls.ToolButton { text: qsTr("Previous"); onClicked: root.nextResult(true) }
            Controls.ToolButton { text: qsTr("Next"); onClicked: root.nextResult(false) }
            Controls.ToolButton { text: qsTr("Close"); onClicked: root.closeFind() }
        }
        Controls.Label { text: qsTr("Active Find displays math as TeX source; selection copies TeX.") }
    }
    Controls.ScrollView {
        id: outline
        objectName: "outlineSidebar"
        palette.window: root.paperColor
        palette.base: root.paperColor
        palette.text: root.textColor
        palette.windowText: root.textColor
        palette.button: root.paperColor
        palette.buttonText: root.textColor
        palette.highlight: root.linkColor
        visible: root.outlineVisible && document.headings.length > 0
        anchors.top: searchRow.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: visible ? Math.min(240, root.width / 3) : 0
        clip: true
        Controls.ScrollBar.horizontal.policy: Controls.ScrollBar.AlwaysOff
        ListView {
            id: outlineList
            model: document.headings
            boundsBehavior: Flickable.StopAtBounds
            currentIndex: -1
            topMargin: 16
            bottomMargin: 16
            delegate: Controls.ItemDelegate {
                id: headingRow
                required property var modelData
                required property int index
                width: outlineList.width
                leftPadding: 20 + (modelData.level - 1) * 12
                rightPadding: 12
                palette.text: root.textColor
                palette.highlight: root.linkColor
                highlighted: activeFocus
                contentItem: Text {
                    text: headingRow.modelData.title
                    textFormat: Text.PlainText
                    elide: Text.ElideRight
                    color: root.textColor
                    opacity: headingRow.modelData.level > 1 ? 0.7 : 1
                    verticalAlignment: Text.AlignVCenter
                    font: headingRow.font
                }
                background: Rectangle { color: headingRow.hovered || headingRow.activeFocus ? Qt.alpha(root.textColor, 0.07) : "transparent"; border.width: headingRow.visualFocus ? 1 : 0; border.color: root.linkColor }
                Rectangle { x: 12; width: root.activatedHeading === headingRow.index ? 3 : 1; height: parent.height; color: root.activatedHeading === headingRow.index ? root.linkColor : root.separatorColor }
                Controls.ToolTip.text: modelData.title
                Controls.ToolTip.visible: hovered
                Accessible.name: modelData.title
                onClicked: { outlineList.currentIndex = index; root.outlineTarget = index; root.linkActivated("#" + modelData.fragmentId) }
                Keys.onDownPressed: outlineList.focusRow(Math.min(outlineList.count - 1, index + 1))
                Keys.onUpPressed: outlineList.focusRow(Math.max(0, index - 1))
                Keys.onReturnPressed: clicked()
                Keys.onEnterPressed: clicked()
            }
            function focusRow(index) {
                currentIndex = index
                positionViewAtIndex(index, ListView.Contain)
                const row = itemAtIndex(index)
                if (row) row.forceActiveFocus()
            }
        }
    }
    Rectangle {
        anchors.left: outline.left
        anchors.top: outline.top
        anchors.bottom: outline.bottom
        width: 1
        visible: outline.visible
        color: root.separatorColor
    }
    Shortcut { sequence: "PgDown"; enabled: document.activeFocus; onActivated: { root.manualScroll(); viewport.scrollTo(viewport.contentY + viewport.height * 0.9) } }
    Shortcut { sequence: "PgUp"; enabled: document.activeFocus; onActivated: { root.manualScroll(); viewport.scrollTo(viewport.contentY - viewport.height * 0.9) } }
    Shortcut { sequence: "Ctrl+Home"; enabled: document.activeFocus; onActivated: { root.manualScroll(); viewport.scrollTo(0) } }
    Shortcut { sequence: "Ctrl+End"; enabled: document.activeFocus; onActivated: { root.manualScroll(); viewport.scrollTo(viewport.contentHeight) } }
    Shortcut { sequence: "F3"; onActivated: root.nextResult(false) }
    Shortcut { sequence: "Shift+F3"; onActivated: root.nextResult(true) }
    Shortcut { sequence: "Escape"; enabled: root.findVisible; onActivated: root.closeFind() }
    Flickable {
        id: viewport
        objectName: "readerViewport"
        anchors.top: searchRow.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: outline.left
        contentWidth: width
        contentHeight: document.height + 48
        clip: true
        Controls.ScrollBar.vertical: Controls.ScrollBar { onPressedChanged: if (pressed) root.manualScroll() }
        onMovementStarted: root.manualScroll()
        onContentYChanged: {
            const range = contentHeight - height
            if (!root.restoring) root.positionChanged(range > 0 ? Math.max(0, Math.min(1, contentY / range)) : 0)
        }
        WheelHandler {
            blocking: false
            onWheel: event => { root.manualScroll(); event.accepted = false }
        }
        function scrollTo(value) { contentY = Math.max(0, Math.min(Math.max(0, contentHeight - height), value)) }
        onContentHeightChanged: if (root.restoring) root.applyPosition()

        MarkdownView {
            id: document
            objectName: "markdownDocument"
            focus: true
            activeFocusOnTab: true
            x: (viewport.width - width) / 2
            y: 24
            width: Math.max(0, Math.min(root.maxColumnWidth, viewport.width - 48))
            selectionEnabled: root.selectionEnabled
            mathEnabled: root.mathEnabled
            syntaxHighlightingEnabled: root.highlightingEnabled
            syntaxTheme: SyntaxTheme { preset: root.dark ? SyntaxTheme.Dark : SyntaxTheme.Light }
            codeBlockWrapEnabled: root.codeWrapping
            codeBlockLineNumbersVisible: root.lineNumbers
            codeBlockHeightLimitEnabled: root.heightLimiting
            codeBlockViewportHeight: viewport.height
            codeBlockHeightRatio: 0.5
            codeBlockMinimumLines: 8
            codeBlockMaximumLines: 30
            codeBlockScrollBar: Component { CodeScrollBar {} }
            baseUrl: root.baseUrl
            resourcePolicy: MarkdownResourcePolicy { allowedFileRoots: root.imageRoots }
            keyboardNavigationEnabled: true
            selectionViewport: viewport
            onSelectionScrollRequested: deltaY => { root.manualScroll(); viewport.scrollTo(viewport.contentY + deltaY) }
            onNavigationRequested: target => {
                if (!root.navigationAllowed) return
                root.restoring = false
                viewport.scrollTo(document.y + target.y - viewport.height / 3)
            }
            onLinkActivated: destination => root.linkActivated(destination)
            searchActive: root.findVisible
            searchQuery: query.text
            searchCaseSensitive: matchCase.checked
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
                codeBlock.verticalPadding: 6
                codeBlock.borderColor: root.separatorColor
                inlineCode.backgroundColor: root.codeBackgroundColor
                table.borderColor: root.separatorColor
            }
            onCopyRequested: function(text) { root.copyRequested(text) }
            onCodeBlockCopyRequested: function(text) { root.copyRequested(text) }
            taskIndicatorsReplaceBullets: true
            taskIndicator: Component {
                TaskIndicator {
                    hostPalette.window: root.paperColor
                    hostPalette.base: root.paperColor
                    hostPalette.button: root.paperColor
                    hostPalette.windowText: root.textColor
                    hostPalette.text: root.textColor
                    hostPalette.buttonText: root.textColor
                    hostPalette.highlight: root.linkColor
                    hostPalette.highlightedText: root.paperColor
                }
            }
            codeBlockActions: Component {
                CodeHeader {
                    palette.window: root.paperColor
                    palette.base: root.paperColor
                    palette.button: root.paperColor
                    palette.windowText: root.textColor
                    palette.text: root.textColor
                    palette.buttonText: root.textColor
                    palette.highlight: root.linkColor
                    palette.highlightedText: root.paperColor
                }
            }
        }
    }
}

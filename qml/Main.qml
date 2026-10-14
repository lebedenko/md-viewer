import QtQuick
import QtQuick.Controls as Controls
import MdViewer

Controls.ApplicationWindow {
    id: window
    width: 960
    height: 720
    visible: true
    title: qsTr("Markdown Viewer")

    ReaderPane {
        anchors.fill: parent
        paperColor: window.palette.base
        textColor: window.palette.text
        linkColor: window.palette.link
        selectionColor: window.palette.highlight
        markdown: "# Markdown Viewer\n\nA comfortable place to read local Markdown documents.\n\n## Reading\n\nSelect text to copy it, or try the copy action on this code block.\n\n```sh\necho 'Hello, Markdown'\n```\n\nFile opening and document navigation are coming next."
        onCopyRequested: function(text) { Clipboard.copyText(text) }
    }
}

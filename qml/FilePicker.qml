pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import MdViewer

Controls.Popup {
    id: root
    required property FolderModel folder
    readonly property var results: visible && folder.revision >= 0 ? folder.search(query.text, 50) : []
    property Item previousFocus: null
    signal fileChosen(url file)

    function choose(index) {
        if (index < 0 || index >= results.length)
            return;
        const file = results[index].url;
        previousFocus = null; // Navigation focuses the document.
        close();
        fileChosen(file);
    }

    modal: true
    focus: true
    padding: 8
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside
    onAboutToShow: {
        previousFocus = parent ? parent.Window.activeFocusItem : null;
        query.clear();
        folder.refresh();
    }
    onOpened: query.forceActiveFocus()
    onClosed: {
        if (previousFocus)
            previousFocus.forceActiveFocus();
        previousFocus = null;
    }
    onResultsChanged: list.currentIndex = results.length > 0 ? 0 : -1

    contentItem: ColumnLayout {
        spacing: 6
        Controls.TextField {
            id: query
            objectName: "pickerQuery"
            Layout.fillWidth: true
            placeholderText: qsTr("Go to file in %1").arg(root.folder.name)
            Accessible.name: qsTr("File name")
            Keys.onDownPressed: list.currentIndex = Math.min(list.count - 1, list.currentIndex + 1)
            Keys.onUpPressed: list.currentIndex = Math.max(0, list.currentIndex - 1)
            onAccepted: root.choose(list.currentIndex)
        }
        ListView {
            id: list
            objectName: "pickerResults"
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 320)
            model: root.results
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)
            delegate: Controls.ItemDelegate {
                id: result
                required property var modelData
                required property int index
                width: list.width
                focusPolicy: Qt.NoFocus
                highlighted: list.currentIndex === index
                contentItem: RowLayout {
                    spacing: 8
                    Controls.Label {
                        text: result.modelData.name
                        textFormat: Text.PlainText
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.maximumWidth: list.width * 0.6
                    }
                    Controls.Label {
                        Layout.fillWidth: true
                        text: result.modelData.directory
                        textFormat: Text.PlainText
                        opacity: 0.7
                        elide: Text.ElideMiddle
                    }
                }
                Accessible.name: result.modelData.directory ? result.modelData.directory + "/" + result.modelData.name : result.modelData.name
                onClicked: root.choose(index)
            }
        }
        Controls.Label {
            objectName: "pickerNote"
            Layout.fillWidth: true
            visible: text !== ""
            text: root.folder.truncated ? qsTr("Results are incomplete: the folder has more than %1 Markdown files.").arg(root.folder.fileCount)
                : root.results.length === 0 ? qsTr("No matching Markdown files.") : ""
            opacity: 0.7
            wrapMode: Text.WordWrap
        }
    }
}

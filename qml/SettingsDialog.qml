import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import MdViewer

Controls.Dialog {
    id: root
    required property AppConfig appConfig
    property var widthPresets: []

    title: qsTr("Settings")
    modal: true
    focus: true
    anchors.centerIn: Controls.Overlay.overlay
    width: Math.min(480, Controls.Overlay.overlay ? Controls.Overlay.overlay.width - 32 : 480)
    standardButtons: Controls.Dialog.Close

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Controls.Label {
            Layout.fillWidth: true
            visible: root.appConfig.error !== ""
            text: "⚠ " + root.appConfig.error
            font.bold: true
            wrapMode: Text.Wrap
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 3
            columnSpacing: 12
            rowSpacing: 12

            Controls.Label {
                text: qsTr("Zoom")
            }
            Controls.Slider {
                id: zoomSlider
                objectName: "zoomSlider"
                Layout.fillWidth: true
                from: 0.5
                to: 3.0
                stepSize: 0.05
                snapMode: Controls.Slider.SnapAlways
                value: root.appConfig.zoom
                onMoved: root.appConfig.zoom = value
            }
            Controls.Label {
                Layout.minimumWidth: 48
                horizontalAlignment: Text.AlignRight
                text: Math.round(root.appConfig.zoom * 100) + "%"
            }

            Controls.Label {
                text: qsTr("Column width")
            }
            Controls.ComboBox {
                id: widthBox
                objectName: "widthBox"
                Layout.fillWidth: true
                Layout.columnSpan: 2
                model: root.widthPresets.map(preset => preset.name)
                currentIndex: root.appConfig.widthPreset
                onActivated: index => root.appConfig.widthPreset = index
            }
        }

        Controls.Label {
            Layout.fillWidth: true
            text: qsTr("Configuration file: %1").arg(root.appConfig.path)
            opacity: 0.7
            font.pointSize: root.font.pointSize * 0.85
            wrapMode: Text.WrapAnywhere
        }
    }
}

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import MdViewer

Controls.Dialog {
    id: root
    required property AppConfig appConfig
    property var widthPresets: []
    property bool mathAvailable: false
    property bool highlightingAvailable: false
    property var themes: []

    title: qsTr("Settings")
    modal: true
    focus: true
    anchors.centerIn: Controls.Overlay.overlay
    width: Math.min(480, Controls.Overlay.overlay ? Controls.Overlay.overlay.width - 32 : 480)
    standardButtons: Controls.Dialog.Close

    height: Math.min(650, Controls.Overlay.overlay ? Controls.Overlay.overlay.height - 32 : 650)
    contentItem: Controls.ScrollView {
        id: settingsScroll
        contentWidth: availableWidth
        clip: true
        ColumnLayout {
            width: settingsScroll.availableWidth
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
                    currentIndex: root.widthPresets.findIndex(preset => preset.value === root.appConfig.widthPreset)
                    onActivated: index => root.appConfig.widthPreset = root.widthPresets[index].value
                }

                Controls.Label {
                    text: qsTr("Theme")
                }
                Controls.ComboBox {
                    id: themeBox
                    objectName: "themeBox"
                    Layout.fillWidth: true
                    Layout.columnSpan: 2
                    model: root.themes.map(theme => theme.name)
                    currentIndex: root.themes.findIndex(theme => theme.value === root.appConfig.theme)
                    onActivated: index => root.appConfig.theme = root.themes[index].value
                }

                Controls.Label {
                    text: qsTr("Body font")
                }
                Controls.ComboBox {
                    id: bodyFontBox
                    objectName: "bodyFontBox"
                    Layout.fillWidth: true
                    Layout.columnSpan: 2
                    model: [qsTr("Default")].concat(Qt.fontFamilies())
                    currentIndex: Math.max(0, model.indexOf(root.appConfig.bodyFont))
                    onActivated: index => root.appConfig.bodyFont = index === 0 ? "" : model[index]
                }

                Controls.Label {
                    text: qsTr("Code font")
                }
                Controls.ComboBox {
                    id: codeFontBox
                    objectName: "codeFontBox"
                    Layout.fillWidth: true
                    Layout.columnSpan: 2
                    model: [qsTr("Default")].concat(Qt.fontFamilies())
                    currentIndex: Math.max(0, model.indexOf(root.appConfig.codeFont))
                    onActivated: index => root.appConfig.codeFont = index === 0 ? "" : model[index]
                }

                Controls.Label {
                    text: qsTr("Block spacing")
                }
                Controls.Slider {
                    id: spacingSlider
                    objectName: "spacingSlider"
                    Layout.fillWidth: true
                    from: 0.5
                    to: 2.0
                    stepSize: 0.1
                    snapMode: Controls.Slider.SnapAlways
                    value: root.appConfig.blockSpacing
                    onMoved: root.appConfig.blockSpacing = value
                }
                Controls.Label {
                    Layout.minimumWidth: 48
                    horizontalAlignment: Text.AlignRight
                    text: Math.round(root.appConfig.blockSpacing * 100) + "%"
                }

                Controls.Label {
                    text: qsTr("Underline links")
                }
                Controls.Switch {
                    id: underlineSwitch
                    objectName: "underlineSwitch"
                    Layout.columnSpan: 2
                    checked: root.appConfig.linkUnderline
                    onToggled: root.appConfig.linkUnderline = checked
                }
            }

            Controls.CheckBox {
                text: qsTr("Select text (eligible formulas stay rendered; copy uses TeX)")
                checked: root.appConfig.selectionEnabled
                onToggled: root.appConfig.selectionEnabled = checked
            }

            Controls.CheckBox {
                text: root.mathAvailable ? qsTr("Render math") : qsTr("Math unavailable: TeX source fallback")
                checked: root.appConfig.mathEnabled
                onToggled: root.appConfig.mathEnabled = checked
            }

            Controls.CheckBox {
                text: root.highlightingAvailable ? qsTr("Syntax highlighting") : qsTr("Highlighting unavailable: plain code fallback")
                checked: root.appConfig.highlightingEnabled
                onToggled: root.appConfig.highlightingEnabled = checked
            }

            Controls.CheckBox {
                text: qsTr("Wrap code")
                checked: root.appConfig.codeWrapping
                onToggled: root.appConfig.codeWrapping = checked
            }

            Controls.CheckBox {
                text: qsTr("Code line numbers")
                checked: root.appConfig.lineNumbers
                onToggled: root.appConfig.lineNumbers = checked
            }

            Controls.CheckBox {
                text: qsTr("Limit code height")
                checked: root.appConfig.heightLimiting
                onToggled: root.appConfig.heightLimiting = checked
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
}

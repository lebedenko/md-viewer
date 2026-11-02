pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import MdViewer

Controls.Dialog {
    id: root
    required property AppConfig appConfig
    required property var readerPane
    property var widthPresets: []
    property bool mathAvailable: false
    property bool highlightingAvailable: false
    property var themes: []
    property int currentTab: 0
    readonly property var tabNames: [qsTr("General"), qsTr("Typography"), qsTr("Headings"), qsTr("Code"), qsTr("Highlighting"), qsTr("Tables"), qsTr("Markdown & Math")]
    // General preferences and detailed overrides share the same row geometry.
    readonly property var generalFields: [
        {
            key: "general.zoom",
            configProperty: "zoom",
            label: qsTr("Zoom"),
            kind: "slider",
            tab: 0,
            defaultValue: 1,
            minimum: 0.5,
            maximum: 3,
            step: 0.05,
            controlName: "zoomSlider"
        },
        {
            key: "general.width",
            configProperty: "widthPreset",
            label: qsTr("Column width"),
            kind: "width",
            tab: 0,
            defaultValue: 1,
            controlName: "widthBox"
        },
        {
            key: "general.theme",
            configProperty: "theme",
            label: qsTr("Theme"),
            kind: "theme",
            tab: 0,
            defaultValue: 0,
            controlName: "themeBox"
        },
        {
            key: "general.spacing",
            configProperty: "blockSpacing",
            label: qsTr("Block spacing multiplier"),
            kind: "slider",
            tab: 1,
            defaultValue: 1,
            minimum: 0.5,
            maximum: 2,
            step: 0.1,
            controlName: "spacingSlider"
        },
        {
            key: "general.wrap",
            configProperty: "codeWrapping",
            label: qsTr("Wrap code"),
            kind: "bool",
            tab: 3,
            defaultValue: false
        },
        {
            key: "general.numbers",
            configProperty: "lineNumbers",
            label: qsTr("Code line numbers"),
            kind: "bool",
            tab: 3,
            defaultValue: true
        },
        {
            key: "general.height",
            configProperty: "heightLimiting",
            label: qsTr("Limit code height"),
            kind: "bool",
            tab: 3,
            defaultValue: true
        },
        {
            key: "general.highlighting",
            configProperty: "highlightingEnabled",
            label: qsTr("Syntax highlighting"),
            kind: "bool",
            tab: 4,
            defaultValue: true
        },
        {
            key: "general.math",
            configProperty: "mathEnabled",
            label: qsTr("Render block math"),
            kind: "bool",
            tab: 6,
            defaultValue: true
        }
    ]
    title: qsTr("Settings")
    modal: true
    focus: true
    anchors.centerIn: Controls.Overlay.overlay
    width: Math.min(760, Controls.Overlay.overlay ? Controls.Overlay.overlay.width - 32 : 760)
    height: Math.min(680, Controls.Overlay.overlay ? Controls.Overlay.overlay.height - 32 : 680)
    topPadding: 16
    bottomPadding: 16
    background: Rectangle {
        objectName: "settingsBackground"
        radius: 12
        color: root.palette.window
        border.color: Qt.alpha(root.palette.windowText, 0.2)
    }
    standardButtons: Controls.Dialog.NoButton
    closePolicy: Controls.Popup.CloseOnEscape
    header: Item {
        implicitHeight: headerRow.implicitHeight + 16
        RowLayout {
            id: headerRow
            anchors.fill: parent
            anchors.topMargin: 12
            anchors.rightMargin: 12
            anchors.bottomMargin: 4
            Controls.Label {
                text: root.title
                font.bold: true
                Layout.fillWidth: true
                leftPadding: 16
            }
            IconButton {
                objectName: "settingsCloseButton"
                label: qsTr("Close settings")
                iconKey: "close"
                Controls.ToolTip.text: ""
                Controls.ToolTip.visible: false
                onClicked: root.close()
            }
        }
    }
    contentItem: ColumnLayout {
        Controls.Label {
            Layout.fillWidth: true
            visible: root.appConfig.error !== ""
            text: "⚠ " + root.appConfig.error
            wrapMode: Text.Wrap
        }
        RowLayout {
            spacing: 20
            Layout.fillWidth: true
            Layout.fillHeight: true
            Controls.ScrollView {
                Layout.preferredWidth: Math.min(160, root.width * 0.28)
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true
                ColumnLayout {
                    width: parent.width
                    Repeater {
                        model: root.tabNames
                        Controls.ItemDelegate {
                            id: tabButton
                            required property int index
                            required property string modelData
                            objectName: "settingsTab" + index
                            Layout.fillWidth: true
                            text: modelData
                            highlighted: root.currentTab === index || activeFocus
                            Accessible.role: Accessible.PageTab
                            Accessible.selected: root.currentTab === index
                            onClicked: root.currentTab = index
                            Keys.onDownPressed: {
                                root.currentTab = Math.min(6, index + 1);
                                nextItemInFocusChain().forceActiveFocus();
                            }
                            Keys.onUpPressed: {
                                root.currentTab = Math.max(0, index - 1);
                                nextItemInFocusChain(false).forceActiveFocus();
                            }
                        }
                    }
                }
            }
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: root.currentTab
                Repeater {
                    model: 7
                    Controls.ScrollView {
                        id: page
                        required property int index
                        leftPadding: 24
                        rightPadding: 24
                        contentWidth: availableWidth
                        clip: true
                        function reveal(item) {
                            const flick = page.contentItem as Flickable;
                            if (!flick)
                                return;
                            const top = item.mapToItem(flick.contentItem, 0, 0).y;
                            if (top < flick.contentY)
                                flick.contentY = top;
                            else if (top + item.height > flick.contentY + flick.height)
                                flick.contentY = Math.max(0, top + item.height - flick.height);
                        }
                        ColumnLayout {
                            width: page.availableWidth
                            spacing: 12
                            Repeater {
                                model: root.generalFields.concat(root.appConfig.settingFields).filter(field => field.tab === page.index)
                                RowLayout {
                                    id: settingRow
                                    spacing: 16
                                    objectName: "setting." + key
                                    required property var modelData
                                    readonly property string key: modelData.key
                                    readonly property var stored: root.appConfig.readerSettings
                                    readonly property bool general: modelData.configProperty !== undefined
                                    readonly property string controlName: modelData.controlName || "control." + key
                                    readonly property bool customized: general ? value !== modelData.defaultValue : stored[key] !== undefined
                                    function update(newValue) {
                                        if (general)
                                            root.appConfig[modelData.configProperty] = newValue;
                                        else
                                            root.appConfig.setReaderSetting(key, newValue);
                                    }
                                    function reset() {
                                        if (general)
                                            update(modelData.defaultValue);
                                        else
                                            root.appConfig.resetReaderSetting(key);
                                    }
                                    readonly property var value: general ? root.appConfig[modelData.configProperty] : stored[key] !== undefined ? stored[key] : (key.startsWith("options.") ? modelData.defaultValue : root.readerPane.defaultSetting(key))
                                    Layout.fillWidth: true
                                    enabled: !["options.extendedAutolinksEnabled", "options.strikethroughEnabled", "options.tablesEnabled", "options.tagFilterEnabled", "options.taskListsEnabled"].includes(key) || !root.readerPane.option("gfmEnabled", true)
                                    Controls.Label {
                                        objectName: "label." + settingRow.key
                                        text: settingRow.modelData.label
                                        Layout.preferredWidth: Math.max(90, page.availableWidth * 0.32)
                                        Layout.minimumWidth: 0
                                        wrapMode: Text.Wrap
                                    }
                                    Loader {
                                        Layout.minimumWidth: 0
                                        Layout.fillWidth: true
                                        sourceComponent: settingRow.modelData.kind === "slider" ? sliderControl : ["width", "theme"].includes(settingRow.modelData.kind) ? generalEnumControl : settingRow.modelData.kind === "bool" ? boolControl : settingRow.modelData.kind === "enum" ? enumControl : settingRow.modelData.kind === "color" ? colorControl : settingRow.modelData.kind === "font" ? fontControl : numberControl
                                        Component {
                                            id: sliderControl
                                            RowLayout {
                                                Controls.Slider {
                                                    Layout.fillWidth: true
                                                    objectName: settingRow.controlName
                                                    from: settingRow.modelData.minimum
                                                    to: settingRow.modelData.maximum
                                                    stepSize: settingRow.modelData.step
                                                    snapMode: Controls.Slider.SnapAlways
                                                    value: Number(settingRow.value)
                                                    Accessible.name: settingRow.modelData.label
                                                    onMoved: settingRow.update(value)
                                                    onActiveFocusChanged: if (activeFocus)
                                                        page.reveal(this)
                                                }
                                                Controls.Label {
                                                    Layout.preferredWidth: 48
                                                    horizontalAlignment: Text.AlignRight
                                                    text: Math.round(settingRow.value * 100) + "%"
                                                }
                                            }
                                        }
                                        Component {
                                            id: generalEnumControl
                                            Controls.ComboBox {
                                                readonly property var choices: settingRow.modelData.kind === "width" ? root.widthPresets : root.themes
                                                objectName: settingRow.controlName
                                                model: choices.map(choice => choice.name)
                                                currentIndex: choices.findIndex(choice => choice.value === settingRow.value)
                                                Accessible.name: settingRow.modelData.label
                                                onActivated: index => settingRow.update(choices[index].value)
                                                onActiveFocusChanged: if (activeFocus)
                                                    page.reveal(this)
                                            }
                                        }
                                        Component {
                                            id: boolControl
                                            Controls.Switch {
                                                checked: Boolean(settingRow.value)
                                                onToggled: settingRow.update(checked)
                                                objectName: settingRow.controlName
                                                Accessible.name: settingRow.modelData.label
                                                onActiveFocusChanged: if (activeFocus)
                                                    page.reveal(this)
                                            }
                                        }
                                        Component {
                                            id: enumControl
                                            Controls.ComboBox {
                                                model: settingRow.modelData.choices
                                                currentIndex: Number(settingRow.value)
                                                onActivated: index => settingRow.update(index)
                                                objectName: settingRow.controlName
                                                Accessible.name: settingRow.modelData.label
                                                onActiveFocusChanged: if (activeFocus)
                                                    page.reveal(this)
                                            }
                                        }
                                        Component {
                                            id: numberControl
                                            Controls.SpinBox {
                                                id: numberEditor
                                                readonly property int factor: settingRow.modelData.kind === "int" ? 1 : 100
                                                from: Math.ceil(settingRow.modelData.minimum * factor)
                                                to: settingRow.modelData.maximum * factor
                                                value: Math.round(Number(settingRow.value) * factor)
                                                editable: true
                                                validator: DoubleValidator {
                                                    bottom: numberEditor.from / numberEditor.factor
                                                    top: numberEditor.to / numberEditor.factor
                                                    decimals: numberEditor.factor === 1 ? 0 : 2
                                                    notation: DoubleValidator.StandardNotation
                                                }
                                                textFromValue: (value, locale) => Number(value / factor).toLocaleString(locale, 'f', factor === 1 ? 0 : 2)
                                                valueFromText: (text, locale) => Math.round(Number.fromLocaleString(locale, text) * factor)
                                                onValueModified: settingRow.update(value / factor)
                                                objectName: settingRow.controlName
                                                Accessible.name: settingRow.modelData.label
                                                onActiveFocusChanged: if (activeFocus)
                                                    page.reveal(this)
                                            }
                                        }
                                        Component {
                                            id: colorControl
                                            Controls.Button {
                                                rightPadding: 44
                                                text: String(settingRow.value)
                                                objectName: settingRow.controlName
                                                Accessible.name: settingRow.modelData.label
                                                onActiveFocusChanged: if (activeFocus)
                                                    page.reveal(this)
                                                onClicked: {
                                                    colorPicker.settingKey = settingRow.key;
                                                    colorPicker.selectedColor = settingRow.value;
                                                    colorPicker.open();
                                                }
                                                Rectangle {
                                                    width: 20
                                                    height: 20
                                                    objectName: "swatch." + settingRow.key
                                                    anchors.right: parent.right
                                                    anchors.rightMargin: 12
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    color: settingRow.value
                                                    border.color: palette.text
                                                }
                                            }
                                        }
                                        Component {
                                            id: fontControl
                                            Controls.Button {
                                                text: settingRow.value.family + " " + settingRow.value.pointSize
                                                objectName: settingRow.controlName
                                                Accessible.name: settingRow.modelData.label
                                                onActiveFocusChanged: if (activeFocus)
                                                    page.reveal(this)
                                                onClicked: {
                                                    fontPicker.settingKey = settingRow.key;
                                                    fontPicker.selectedFont = settingRow.value;
                                                    fontPicker.open();
                                                }
                                            }
                                        }
                                    }
                                    IconButton {
                                        id: resetButton
                                        Layout.alignment: Qt.AlignVCenter
                                        onActiveFocusChanged: if (activeFocus)
                                            page.reveal(this)
                                        objectName: "reset." + settingRow.key
                                        iconKey: "reset"
                                        label: qsTr("Reset %1").arg(settingRow.modelData.label)
                                        enabled: settingRow.customized
                                        hoverEnabled: enabled
                                        Controls.ToolTip.text: enabled ? label : ""
                                        Controls.ToolTip.visible: enabled && hovered
                                        Binding {
                                            target: resetButton.background
                                            property: "visible"
                                            when: !resetButton.enabled
                                            value: false
                                        }
                                        onClicked: settingRow.reset()
                                    }
                                }
                            }
                            Controls.Label {
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                                visible: page.index === 6 || page.index === 4
                                text: page.index === 6 ? (root.mathAvailable ? qsTr("GFM forces its five extensions on. Individual values are retained until GFM is off.") : qsTr("Math unavailable: TeX source fallback. GFM forces its five extensions on.")) : (root.highlightingAvailable ? qsTr("Syntax palette follows reader theme in Automatic mode.") : qsTr("Highlighting unavailable: plain code fallback"))
                            }
                        }
                    }
                }
            }
        }
        Controls.Label {
            Layout.fillWidth: true
            text: qsTr("Configuration file: %1").arg(root.appConfig.path)
            opacity: 0.7
            wrapMode: Text.WrapAnywhere
        }
    }
    ColorDialog {
        id: colorPicker
        property string settingKey
        options: ColorDialog.ShowAlphaChannel
        onAccepted: root.appConfig.setReaderSetting(settingKey, selectedColor.toString())
    }
    FontDialog {
        id: fontPicker
        property string settingKey
        onAccepted: root.appConfig.setReaderSetting(settingKey, selectedFont)
    }
}

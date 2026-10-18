import QtQuick
import QtQuick.Controls as Controls

Controls.ScrollBar {
    id: root
    required property var requestPosition
    onPositionChanged: if (pressed) requestPosition(position)
}

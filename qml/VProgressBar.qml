import QtQuick
import QtQuick.Controls
import vqt

Item {
    id: control

    property real max: 100
    property real min: 0
    property string name
    property real value: 50
    property var valueToString: x => x

    implicitHeight: nameLabel.implicitHeight + (valueLabel.font.pixelSize * 0.4)
    implicitWidth: nameLabel.implicitWidth + valueLabel.implicitWidth + valueLabel.font.pixelSize

    VLabel {
        id: nameLabel

        text: control.name
        y: -3
    }

    VLabel {
        id: valueLabel

        anchors.right: control.right
        font.bold: true
        font.italic: true
        text: control.valueToString(control.value)
        y: nameLabel.y
    }

    Rectangle {
        anchors.margins: 3
        anchors.top: nameLabel.bottom
        color: Qt.darker("#c2b05a", 1.9)
        height: 5
        width: control.width
    }

    Rectangle {
        anchors.margins: 3
        anchors.top: nameLabel.bottom
        color: "#c2b05a"
        height: 5
        width: control.width * Math.min(1.0, ((control.value - control.min) / (control.max - control.min)))
    }
}

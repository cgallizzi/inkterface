import QtQuick
import QtQuick.Controls
import vqt

Item {
    id: control
    implicitHeight: nameLabel.implicitHeight + (valueLabel.font.pixelSize * 0.4)
    implicitWidth: nameLabel.implicitWidth + valueLabel.implicitWidth + valueLabel.font.pixelSize

    property string name
    property real value: 50
    property real min: 0
    property real max: 100

    property var valueToString: (x) => x

    VLabel {
        id: nameLabel
        y: -3
        text: control.name
    }

    VLabel {
        id: valueLabel
        y: nameLabel.y
        text: control.valueToString(control.value)
        font.bold: true
        font.italic: true
        anchors.right: control.right
    }

    Rectangle {
        width: control.width
        height: 5
        color: Qt.darker("#c2b05a", 1.9)
        anchors.top: nameLabel.bottom
        anchors.margins: 3
    }

    Rectangle {
        width: control.width * Math.min(1.0, ((control.value - control.min) / (control.max - control.min)))
        height: 5
        color: "#c2b05a"
        anchors.top: nameLabel.bottom
        anchors.margins: 3
    }
}

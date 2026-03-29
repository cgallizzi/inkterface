import QtQuick
import QtQuick.Controls

GroupBox {
    id: control

    bottomPadding: 10
    leftPadding: 10
    rightPadding: 14
    topPadding: !!control.title ? label.implicitHeight + 10 : 6

    background: VRect {
        flipped: true
        height: parent.height - y
        width: parent.width
        y: !!control.title ? label.implicitHeight + 3 : 0
    }
    label: VLabel {
        elide: Label.ElideRight
        font.bold: true
        font.italic: true
        font.pixelSize: Qt.application.font.pixelSize * 1.1
        text: control.title
        width: control.availableWidth
    }
}

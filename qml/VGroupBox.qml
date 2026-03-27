import QtQuick
import QtQuick.Controls

GroupBox {
    id: control
    topPadding: !!control.title ?  label.implicitHeight + 10 : 6
    bottomPadding: 10
    leftPadding: 10
    rightPadding: 14
    background: VRect {
        y: !!control.title ? label.implicitHeight + 3 : 0
        width: parent.width
        height: parent.height - y
        flipped: true
    }
    label: VLabel {
        width: control.availableWidth
        text: control.title
        font.pixelSize: Qt.application.font.pixelSize * 1.1
        font.bold: true
        font.italic: true
        elide: Label.ElideRight
    }
}


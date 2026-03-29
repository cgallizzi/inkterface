import QtQuick
import QtQuick.Controls
import vqt

VRect {
    id: control

    property alias font: titleLabel.font
    property alias fontColor: titleLabel.color
    property alias title: titleLabel.text
    property string value

    flipped: true
    implicitHeight: Math.max(titleLabel.height, valueLabel.height) + (control.depth * 2) + 3
    implicitWidth: 265

    VLabel {
        id: titleLabel

        anchors.left: parent.left
        anchors.leftMargin: parent.depth + 5
        anchors.right: valueLabel.left
        anchors.rightMargin: 10
        anchors.top: parent.top
        anchors.topMargin: parent.depth
        elide: Label.ElideRight
    }

    VLabel {
        id: valueLabel

        anchors.right: parent.right
        anchors.rightMargin: parent.depth + 5
        anchors.top: parent.top
        anchors.topMargin: parent.depth
        color: titleLabel.color
        font: titleLabel.font
        text: `<i><b>${control.value}</b></i>`
    }
}

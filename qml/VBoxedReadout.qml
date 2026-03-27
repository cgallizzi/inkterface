import QtQuick
import QtQuick.Controls
import vqt

VRect {
    id: control
    flipped: true
    implicitWidth: 265
    implicitHeight: Math.max(titleLabel.height, valueLabel.height) + (control.depth * 2) + 3

    property alias font: titleLabel.font
    property alias fontColor: titleLabel.color
    property alias title: titleLabel.text
    property string value

    VLabel {
        id: titleLabel
        elide: Label.ElideRight
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: valueLabel.left
        anchors.topMargin: parent.depth
        anchors.leftMargin: parent.depth + 5
        anchors.rightMargin: 10
    }

    VLabel {
        id: valueLabel
        text: `<i><b>${control.value}</b></i>`
        color: titleLabel.color
        font: titleLabel.font
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: parent.depth
        anchors.rightMargin: parent.depth + 5
    }
}

import QtQuick
import QtQuick.Controls

VRect {
    id: control
    flipped: control.down
    opacity: control.enabled ? 1.0 : 0.5
    implicitWidth: label.implicitWidth + (4 * control.depth)
    implicitHeight: label.implicitHeight + (2 * control.depth)

    property alias text: label.text
    property alias font: label.font
    property alias topPadding: label.topPadding
    property alias leftPadding: label.leftPadding
    property alias rightPadding: label.rightPadding
    property alias bottomPadding: label.bottomPadding
    property alias horizontalAlignment: label.horizontalAlignment
    property alias verticalAlignment: label.verticalAlignment
    property alias containsPress: mouseArea.containsPress

    property bool down: mouseArea.containsPress

    signal clicked
    signal pressed
    signal released

    VLabel {
        id: label
        color: control.fgColor
        topPadding: 7
        bottomPadding: 7
        leftPadding: 10
        rightPadding: 10
        horizontalAlignment: Label.AlignHCenter
        verticalAlignment: Label.AlignVCenter
        width: parent.width
        height: parent.height
    }

    MouseArea {
        id: mouseArea
        width: parent.width
        height: parent.height

        onClicked: {
            control.forceActiveFocus()
            control.clicked()
        }
        onPressed: {
            control.forceActiveFocus()
            control.pressed()
        }
        onReleased: control.released()
    }
}

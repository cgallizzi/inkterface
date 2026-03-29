import QtQuick
import QtQuick.Controls

VRect {
    id: control

    property alias bottomPadding: label.bottomPadding
    property alias containsPress: mouseArea.containsPress
    property bool down: mouseArea.containsPress
    property alias font: label.font
    property alias horizontalAlignment: label.horizontalAlignment
    property alias leftPadding: label.leftPadding
    property alias rightPadding: label.rightPadding
    property alias text: label.text
    property alias topPadding: label.topPadding
    property alias verticalAlignment: label.verticalAlignment

    signal clicked
    signal pressed
    signal released

    flipped: control.down
    implicitHeight: label.implicitHeight + (2 * control.depth)
    implicitWidth: label.implicitWidth + (4 * control.depth)
    opacity: control.enabled ? 1.0 : 0.5

    VLabel {
        id: label

        bottomPadding: 7
        color: control.fgColor
        height: parent.height
        horizontalAlignment: Label.AlignHCenter
        leftPadding: 10
        rightPadding: 10
        topPadding: 7
        verticalAlignment: Label.AlignVCenter
        width: parent.width
    }

    MouseArea {
        id: mouseArea

        height: parent.height
        width: parent.width

        onClicked: {
            control.forceActiveFocus();
            control.clicked();
        }
        onPressed: {
            control.forceActiveFocus();
            control.pressed();
        }
        onReleased: control.released()
    }
}

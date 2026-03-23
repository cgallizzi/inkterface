import QtQuick
import QtQuick.Controls

TextField {
    id: control
    topPadding: 10
    bottomPadding: 10
    leftPadding: 10
    rightPadding: 10
    color: background.fgColor
    selectionColor: background.hiColor
    placeholderTextColor: background.hiColor
    background: VRect {
        style: control.acceptableInput ? 0 : 1
        flipped: true
        bgColor: dbgColor
        implicitWidth: control.implicitWidth
        implicitHeight: control.implicitHeight
    }

    onActiveFocusChanged: {
        if (activeFocus) {
            control.selectAll()
        }
    }
}

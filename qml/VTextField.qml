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
    opacity: control.enabled ? 1.0 : 0.5

    // keyboard mode for steamworks floating keyboard
    //    -1 = disabled (also disabled when steamworks is unusable)
    //    0 = single line, enter dismisses keyboard
    //    1 = multi line, user needs to dismiss
    //    2 = email, special email mode (easier @ symbol, etc.)
    //    3 = numeric, numeric keypad (doesn't seem to do anything)
    property int keyboardMode: 0

    onActiveFocusChanged: {
        if (activeFocus) {
            control.selectAll()
            if (steamworks.ready && control.keyboardMode >= 0) {
                var pos = control.mapToGlobal(0, 0)
                steamworks.openKeyboard(control.keyboardMode, pos.x, pos.y, control.width, control.height)
            }
        }
    }
}

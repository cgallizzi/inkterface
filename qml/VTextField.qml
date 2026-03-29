import QtQuick
import QtQuick.Controls

TextField {
    id: control

    // keyboard mode for steamworks floating keyboard
    //    -1 = disabled (also disabled when steamworks is unusable)
    //    0 = single line, enter dismisses keyboard
    //    1 = multi line, user needs to dismiss
    //    2 = email, special email mode (easier @ symbol, etc.)
    //    3 = numeric, numeric keypad (doesn't seem to do anything)
    property int keyboardMode: 0

    bottomPadding: 10
    color: background.fgColor
    leftPadding: 10
    opacity: control.enabled ? 1.0 : 0.5
    placeholderTextColor: background.hiColor
    rightPadding: 10
    selectionColor: background.hiColor
    topPadding: 10

    background: VRect {
        bgColor: dbgColor
        flipped: true
        implicitHeight: control.implicitHeight
        implicitWidth: control.implicitWidth
        style: control.acceptableInput ? 0 : 1
    }

    onActiveFocusChanged: {
        if (activeFocus) {
            control.selectAll();
            if (steamworks.ready && control.keyboardMode >= 0) {
                var pos = control.mapToGlobal(0, 0);
                steamworks.openKeyboard(control.keyboardMode, pos.x, pos.y, control.width, control.height);
            }
        }
    }
}

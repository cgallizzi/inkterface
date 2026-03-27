import QtQuick
import QtQuick.Controls

VButton {
    id: control
    text: debounce.running ? control.confirmText : control.normalText

    property string normalText: "<placeholder>"
    property string confirmText: "Are you sure?"
    property alias interval: debounce.interval

    signal confirmed

    onClicked: {
        if (debounce.running) {
            debounce.stop()
            control.confirmed()
        } else {
            debounce.restart()
        }
    }

    Timer {
        id: debounce
        interval: 3000
        running: false
        repeat: false
    }
}

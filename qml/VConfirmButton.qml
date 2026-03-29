import QtQuick
import QtQuick.Controls

VButton {
    id: control

    property string confirmText: "Are you sure?"
    property alias interval: debounce.interval
    property string normalText: "<placeholder>"

    signal confirmed

    text: debounce.running ? control.confirmText : control.normalText

    onClicked: {
        if (debounce.running) {
            debounce.stop();
            control.confirmed();
        } else {
            debounce.restart();
        }
    }

    Timer {
        id: debounce

        interval: 3000
        repeat: false
        running: false
    }
}

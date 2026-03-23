import QtQuick
import QtQuick.Controls

VRect {
    id: control
    visible: timer.running
    implicitWidth: label.implicitWidth + (4 * control.depth)
    implicitHeight: label.implicitHeight + (2 * control.depth)

    property alias text: label.text
    property alias duration: timer.interval

    function showError(message) {
        control.style = 1
        control.show(message)
    }

    function showNotification(message) {
        control.style = 2
        control.show(message)
    }

    function show(message) {
        label.text = message
        if (!label.text) {
            timer.stop()
        } else {
            timer.restart()
        }
    }

    Timer {
        id: timer
        interval: 5000
        repeat: false
        running: false
    }

    VLabel {
        id: label
        wrapMode: Label.WrapAtWordBoundaryOrAnywhere
        topPadding: 7
        bottomPadding: 7
        leftPadding: 10
        rightPadding: 10
        horizontalAlignment: Label.AlignLeft
        verticalAlignment: Label.AlignVCenter
        width: control.width
        height: control.height
    }
}

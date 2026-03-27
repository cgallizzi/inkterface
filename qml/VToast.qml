import QtQuick
import QtQuick.Controls

VRect {
    id: control
    visible: timer.running
    implicitWidth: label.implicitWidth + (4 * control.depth)
    implicitHeight: label.implicitHeight + (2 * control.depth)
    flipped: clipboardTimer.running

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

    Timer {
        id: clipboardTimer
        interval: 100
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

    TextEdit {
        id: clipboard
        visible: false
    }

    MouseArea {
        enabled: timer.running
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        anchors.fill: parent

        onClicked: function (event) {
            if (event.button === Qt.RightButton) {
                clipboardTimer.start()
                clipboard.text = control.text
                clipboard.selectAll()
                clipboard.copy()
            } else {
                timer.stop()
            }
        }
    }
}

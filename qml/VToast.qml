import QtQuick
import QtQuick.Controls

VRect {
    id: control

    property alias duration: timer.interval
    property alias text: label.text

    function show(message) {
        label.text = message;
        if (!label.text) {
            timer.stop();
        } else {
            timer.restart();
        }
    }

    function showError(message) {
        control.style = 1;
        control.show(message);
    }

    function showNotification(message) {
        control.style = 2;
        control.show(message);
    }

    flipped: clipboardTimer.running
    implicitHeight: label.implicitHeight + (2 * control.depth)
    implicitWidth: label.implicitWidth + (4 * control.depth)
    visible: timer.running

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

        bottomPadding: 7
        height: control.height
        horizontalAlignment: Label.AlignLeft
        leftPadding: 10
        rightPadding: 10
        topPadding: 7
        verticalAlignment: Label.AlignVCenter
        width: control.width
        wrapMode: Label.WrapAtWordBoundaryOrAnywhere
    }

    TextEdit {
        id: clipboard

        visible: false
    }

    MouseArea {
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        anchors.fill: parent
        enabled: timer.running

        onClicked: function (event) {
            if (event.button === Qt.RightButton) {
                clipboardTimer.start();
                clipboard.text = control.text;
                clipboard.selectAll();
                clipboard.copy();
            } else {
                timer.stop();
            }
        }
    }
}

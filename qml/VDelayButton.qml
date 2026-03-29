import QtQuick

MouseArea {
    id: control

    property bool active: false
    property color bgColor: "#c2b05a"
    property real completeOpacity: 0.4
    property int delay: 600
    property real initialOpacity: 0.2
    property int initialWidth: 5
    property int progress: 0

    signal longPress
    signal shortPress

    implicitHeight: 80
    implicitWidth: 200

    onContainsMouseChanged: {
        if (!control.containsMouse) {
            progress = 0;
        }
    }
    onReleased: {
        if (control.containsMouse && (control.delay < 0 || control.progress < control.delay)) {
            control.shortPress();
        }
        progress = 0;
    }

    Timer {
        interval: 33
        repeat: true
        running: control.progress < control.delay && control.containsPress

        onTriggered: {
            control.progress = Math.min(control.delay, control.progress + interval);
            if (control.progress >= control.delay) {
                control.longPress();
            }
        }
    }

    Rectangle {
        color: control.bgColor
        height: control.height
        opacity: control.progress < control.delay ? control.initialOpacity : control.completeOpacity
        width: (control.width - control.initialWidth) * (control.progress / control.delay)
        x: control.initialWidth
        y: 0
    }

    Rectangle {
        color: control.bgColor
        height: control.height
        opacity: control.active ? 1.0 : control.initialOpacity
        width: control.initialWidth
        x: 0
        y: 0
    }
}

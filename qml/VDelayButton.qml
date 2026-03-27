import QtQuick

MouseArea {
    id: control
    implicitHeight: 80
    implicitWidth: 200

    signal shortPress
    signal longPress

    property color bgColor: "#c2b05a"
    property bool active: false
    property int delay: 600
    property int progress: 0
    property int initialWidth: 5
    property real initialOpacity: 0.2
    property real completeOpacity: 0.4

    onReleased: {
        if (control.containsMouse && (control.delay < 0
                                      || control.progress < control.delay)) {
            control.shortPress()
        }
        progress = 0
    }

    onContainsMouseChanged: {
        if (!control.containsMouse) {
            progress = 0
        }
    }

    Timer {
        running: control.progress < control.delay && control.containsPress
        interval: 33
        repeat: true

        onTriggered: {
            control.progress = Math.min(control.delay, control.progress + interval)
            if (control.progress >= control.delay) {
                control.longPress()
            }
        }
    }

    Rectangle {
        opacity: control.progress < control.delay ? control.initialOpacity : control.completeOpacity
        color: control.bgColor
        x: control.initialWidth
        y: 0
        width: (control.width - control.initialWidth) * (control.progress / control.delay)
        height: control.height
    }

    Rectangle {
        opacity: control.active ? 1.0 : control.initialOpacity
        color: control.bgColor
        x: 0
        y: 0
        width: control.initialWidth
        height: control.height
    }
}

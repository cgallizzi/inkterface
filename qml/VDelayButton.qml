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
        interval: 50
        repeat: true

        onTriggered: {
            control.progress = Math.min(control.delay,
                                        control.progress + interval)
            if (control.progress >= control.delay) {
                control.longPress()
            }
        }
    }

    Rectangle {
        opacity: control.progress < control.delay ? 0.2 : 0.4
        color: control.bgColor
        x: 5
        y: 0
        width: (control.width - 5) * (control.progress / control.delay)
        height: control.height
    }

    Rectangle {
        opacity: control.active ? 1.0 : 0.2
        color: control.bgColor
        x: 0
        y: 0
        width: 5
        height: control.height
    }
}

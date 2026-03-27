import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt

ApplicationWindow {
    id: rootWindow
    title: `${Qt.application.displayName} (v${Qt.application.version})`
    visible: true
    // visibility: Qt.platform.os === "linux" ? Window.FullScreen : Window.Windowed
    minimumWidth: 1280
    minimumHeight: 720
    color: "#4d5845"

    Item {
        id: focusThief
        width: 0
        height: 0
    }

    MouseArea {
        anchors.fill: parent

        onClicked: focusThief.forceActiveFocus()
    }

    Flow {
        spacing: 10
        anchors.centerIn: parent

        VBoxedReadout {
            title: "CPU dC"
            value: "120dC"
        }

        VSparkline {
            title: "GPU dC"
            value: "50dC"
        }
    }

    VButton {
        text: exitDebounce.running ? "Are you sure?" : "Exit App"
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 10

        onClicked: {
            if (exitDebounce.running) {
                Qt.callLater(Qt.quit)
            } else {
                exitDebounce.restart()
            }
        }

        Timer {
            id: exitDebounce
            interval: 3000
            running: false
            repeat: false
        }
    }

    VToast {
        id: toast
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
    }
}

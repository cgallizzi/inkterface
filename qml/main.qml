import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt

ApplicationWindow {
    id: rootWindow

    color: "#4d5845"
    minimumHeight: 720
    // visibility: Qt.platform.os === "linux" ? Window.FullScreen : Window.Windowed
    minimumWidth: 1280
    title: `${Qt.application.displayName} (v${Qt.application.version})`
    visible: true

    Item {
        id: focusThief

        height: 0
        width: 0
    }

    MouseArea {
        anchors.fill: parent

        onClicked: focusThief.forceActiveFocus()
    }

    Flow {
        anchors.centerIn: parent
        spacing: 10

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
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 10
        text: exitDebounce.running ? "Are you sure?" : "Exit App"

        onClicked: {
            if (exitDebounce.running)
                Qt.callLater(Qt.quit);
            else
                exitDebounce.restart();
        }

        Timer {
            id: exitDebounce

            interval: 3000
            repeat: false
            running: false
        }
    }

    VToast {
        id: toast

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 10
        anchors.right: parent.right
    }
}

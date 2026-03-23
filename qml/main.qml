import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt


/* Colors
 *  most are shades of greenish (from dark to light)
 *  btm-right dark edge = #242f1f
 *  bg/dark btn/etc = #3e4638 (dark green)
 *  light btn = #4c5844 (light green)
 *  lighter btn/left edge/alt txt = #697464 (lighter desaturated green)
 *  top light edge = #7d8576
 *
 *  there is also a text color
 *  txt = #c2b05a (yellowish, light)
 */
ApplicationWindow {
    id: rootWindow
    title: `${Qt.application.displayName} (v${Qt.application.version})`
    visible: true
    visibility: Qt.platform.os === "linux" ? Window.FullScreen : Window.Windowed
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

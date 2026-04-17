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
    title: `${Qt.application.displayName} (${Qt.application.version})`
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

    ListView {
        height: 180
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        spacing: 10
        orientation: ListView.Horizontal

        model: frunkFinder.frunks

        delegate: Rectangle {
            color: "#111"
            radius: 3
            height: ListView.view.height
            width: 190

            Rectangle {
                color: "#ddd"
                radius: 3
                anchors.fill: parent
                anchors.margins: 10

                Rectangle {
                    id: logoRect
                    color: "#111"
                    radius: 3
                    height: parent.height * 0.2
                    width: height
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.margins: 3

                    Rectangle {
                        color: "#ddd"
                        height: parent.height * 0.5
                        width: height
                        radius: height
                        anchors.centerIn: parent
                    }
                }

                Label {
                    text: modelData.name
                    font.pixelSize: logoRect.height * 0.5
                    anchors.left: logoRect.right
                    anchors.top: parent.top
                    anchors.margins: 3
                }
            }

            // title: modelData.name
            // value: modelData.rssi
        }
    }

    VConfirmButton {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10
        normalText: "Exit App"

        onConfirmed: Qt.callLater(Qt.quit)
    }

    VToast {
        id: toast

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 10
        anchors.right: parent.right
    }
}

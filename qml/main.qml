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

    FrunkPanel {
        height: 390
        width: 510
        opacity: 0.2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.margins: 20
    }

    VLabel {
        text: "Nearby Devices:"
        anchors.left: parent.left
        anchors.bottom: frunkList.top
        anchors.margins: 20
        anchors.bottomMargin: 10
    }

    ListView {
        id: frunkList
        height: 130
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        spacing: 10
        orientation: ListView.Horizontal

        model: frunkFinder.frunks

        delegate: FrunkPanel {
            name: modelData.name
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

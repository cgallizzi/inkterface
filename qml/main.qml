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
        height: 120
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        spacing: 10
        orientation: ListView.Horizontal

        model: ListModel {
            ListElement {
                thing: "Some text"
            }
            ListElement {
                thing: "Second Thing"
            }
            ListElement {
                thing: "Third thing"
            }
            ListElement {
                thing: "More things"
            }
            ListElement {
                thing: "More things"
            }
            ListElement {
                thing: "More things"
            }
            ListElement {
                thing: "More things"
            }
            ListElement {
                thing: "More things"
            }
            ListElement {
                thing: "More things"
            }
        }

        delegate: VBoxedReadout {
            title: thing
            value: "12.34"
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

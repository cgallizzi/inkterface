import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt

ApplicationWindow {
    id: rootWindow

    color: "#4d5845"
    height: Math.min(720, Screen.height)
    minimumHeight: Math.min(720, Screen.height)
    minimumWidth: Math.min(720, Screen.width)
    title: `${Qt.application.displayName} (${Qt.application.version})`
    visible: true
    width: Math.min(1280, Screen.width)

    Component.onCompleted: {
        if (settings.value("panelName") === "") {
            pageLoader.source = "PanelListPage.qml";
        } else {
            pageLoader.source = "PanelConfigPage.qml";
        }
    }

    // visibility: Qt.platform.os === "linux" ? Window.FullScreen : Window.Windowed

    Item {
        id: focusThief

        height: 0
        width: 0
    }

    MouseArea {
        anchors.fill: parent

        onClicked: focusThief.forceActiveFocus()
    }

    Settings {
        id: settings

    }

    Loader {
        id: pageLoader

        anchors.fill: parent
    }

    Connections {
        function onError(message) {
            toast.showError(message);
        }

        function onNotification(message) {
            toast.showNotification(message);
        }

        function onPanelCleared() {
            pageLoader.source = "PanelListPage.qml";
        }

        function onPanelSelected() {
            pageLoader.source = "PanelConfigPage.qml";
        }

        target: pageLoader.item
    }

    VToast {
        id: toast

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 10
        anchors.right: parent.right
    }
}

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt

ApplicationWindow {
    id: rootWindow

    color: "#4d5845"
    height: 720
    minimumHeight: 720
    minimumWidth: 720
    title: `${Qt.application.displayName} (${Qt.application.version})`
    visible: true
    width: 1280

    Component.onCompleted: {
        if (settings.value("frunkName") === "") {
            pageLoader.source = "FrunkListPage.qml";
        } else {
            pageLoader.source = "FrunkConfigPage.qml";
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

        function onFrunkCleared() {
            pageLoader.source = "FrunkListPage.qml";
        }

        function onFrunkSelected() {
            pageLoader.source = "FrunkConfigPage.qml";
        }

        function onNotification(message) {
            toast.showNotification(message);
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt

Image {
    id: control

    property alias name: nameLabel.text
    property alias rssi: rssiLabel.text
    property bool supported: true
    property alias version: versionLabel.text

    source: "qrc:/vqt/resources/panel.svg"
    sourceSize.height: height

    VLabel {
        id: nameLabel

        anchors.left: parent.left
        anchors.leftMargin: control.width * 0.258
        anchors.top: parent.top
        anchors.topMargin: control.height * 0.133
        color: "#333"
        font.pixelSize: control.height * 0.102
    }

    VLabel {
        id: rssiLabel

        anchors.bottom: parent.bottom
        anchors.bottomMargin: control.height * 0.123
        anchors.right: parent.right
        anchors.rightMargin: control.width * 0.104
        color: "#333"
        font.pixelSize: control.height * 0.057
    }

    VLabel {
        id: versionLabel

        anchors.bottom: parent.bottom
        anchors.bottomMargin: control.height * 0.123
        anchors.left: parent.left
        anchors.leftMargin: control.width * 0.104
        color: "#333"
        font.pixelSize: control.height * 0.057
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.5
        visible: !control.supported
    }

    VLabel {
        anchors.centerIn: parent
        color: "#b74623"
        font.pixelSize: control.height * 0.15
        rotation: 45
        text: "UNSUPPORTED"
        visible: !control.supported
    }
}

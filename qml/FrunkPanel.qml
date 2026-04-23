import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt

Rectangle {
    color: "#111"
    radius: 3
    height: 130
    width: 170

    property alias name: nameLabel.text

    Rectangle {
        color: "#ddd"
        radius: 3
        anchors.fill: parent
        anchors.margins: parent.height * 0.08

        Rectangle {
            id: logoRect
            color: "#111"
            radius: 3
            height: parent.height * 0.2
            width: height
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: parent.height * 0.02

            Rectangle {
                color: "#ddd"
                height: parent.height * 0.55
                width: height
                radius: height
                anchors.centerIn: parent
            }
        }

        Label {
            id: nameLabel
            text: modelData.name
            font.pixelSize: logoRect.height * 0.72
            anchors.verticalCenter: logoRect.verticalCenter
            anchors.left: logoRect.right
            anchors.margins: parent.height * 0.02
        }
    }
}

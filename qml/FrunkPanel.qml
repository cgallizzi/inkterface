import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt

Image {
    id: control
    source: "qrc:/vqt/resources/frunk.svg"
    sourceSize.height: height
    sourceSize.width: width

    property alias name: nameLabel.text
    property alias rssi: rssiLabel.text
    property alias version: versionLabel.text
    property bool supported: true

     VLabel {
         id: nameLabel
         color: "#333"
         font.pixelSize: control.height * 0.102
         anchors.left: parent.left
         anchors.top: parent.top
         anchors.leftMargin: control.width * 0.258
         anchors.topMargin: control.height * 0.133
     }

     VLabel {
         id: rssiLabel
         color: "#333"
         font.pixelSize: control.height * 0.057
         anchors.right: parent.right
         anchors.bottom: parent.bottom
         anchors.rightMargin: control.width * 0.104
         anchors.bottomMargin: control.height * 0.123
     }

     VLabel {
         id: versionLabel
         color: "#333"
         font.pixelSize: control.height * 0.057
         anchors.left: parent.left
         anchors.bottom: parent.bottom
         anchors.leftMargin: control.width * 0.104
         anchors.bottomMargin: control.height * 0.123
     }

     Rectangle {
         visible: !control.supported
         color: "black"
         opacity: 0.5
         anchors.fill: parent
     }

     VLabel {
         visible: !control.supported
         text: "UNSUPPORTED"
         rotation: 45
         color: "#9d391a"
         font.pixelSize: control.height * 0.15
         anchors.centerIn: parent
     }
}

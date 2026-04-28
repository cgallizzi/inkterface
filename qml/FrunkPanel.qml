import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt

Image {
    id: control
    source: "qrc:/vqt/resources/frunk.svg"
    // color: "#111"
    // radius: 3
    // height: 130
    // width: 170
    sourceSize.height: height
    sourceSize.width: width

    property alias name: nameLabel.text

    // Rectangle {
    //     color: "#ddd"
    //     radius: 3
    //     anchors.fill: parent
    //     anchors.margins: parent.height * 0.08

    //     Rectangle {
    //         id: logoRect
    //         color: "#111"
    //         radius: 3
    //         height: parent.height * 0.2
    //         width: height
    //         anchors.left: parent.left
    //         anchors.top: parent.top
    //         anchors.margins: parent.height * 0.02

    //         Rectangle {
    //             color: "#ddd"
    //             height: parent.height * 0.55
    //             width: height
    //             radius: height
    //             anchors.centerIn: parent
    //         }
    //     }

         Label {
             id: nameLabel
             font.pixelSize: control.height * 0.102
             anchors.left: parent.left
             anchors.top: parent.top
             anchors.leftMargin: control.width * 0.26
             anchors.topMargin: control.height * 0.16
             // anchors.verticalCenter: logoRect.verticalCenter
             // anchors.left: logoRect.right
             // anchors.margins: parent.height * 0.02
         }
    // }
}

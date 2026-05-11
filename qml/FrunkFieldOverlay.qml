import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt

MouseArea {
    id: control

    property var field

    onClicked: control.finished()

    signal finished

    Rectangle {
        anchors.fill: parent
        color: "#000"
        opacity: 0.7
    }

    VRect {
        height: control.height * 0.8
        width: control.width * 0.75
        anchors.centerIn: parent

        MouseArea { anchors.fill: parent }

        ColumnLayout {
            spacing: 15
            anchors.fill: parent
            anchors.margins: 20

            VLabel {
                text: "Select data to display:"
                font.bold: true
                font.italic: true
                Layout.fillWidth: true
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10
                model: control.field.depth > 0 ? frunkState.collectors.filter((x) => x.hasDbl) : frunkState.collectors
                clip: true
                header: Item {
                    height: 10
                }
                footer: Item {
                    height: 10
                }

                delegate: VBevelRect {
                    style: modelData === control.field.collector ? 2 : 0
                    width: ListView.view.width
                    height: 100

                    VLabel {
                        text: modelData.displayName
                        font.bold: true
                        font.italic: true
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: 15
                    }

                    VLabel {
                        text: modelData.description
                        elide: Label.ElideRight
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 15
                    }

                    VLabel {
                        id: valueLabel
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: 15

                        Component.onCompleted: valueLabel.text = modelData.getStr() || "N/A"
                    }

                    MouseArea {
                        anchors.fill: parent

                        onClicked: {
                            control.field.collector = modelData
                            control.finished()
                        }
                    }
                }

                Rectangle {
                    height: 10
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top

                    gradient: Gradient {
                        GradientStop {
                            color: "#4d5845"
                            position: 0.15
                        }

                        GradientStop {
                            color: "transparent"
                            position: 1.0
                        }
                    }
                }

                Rectangle {
                    height: 10
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right

                    gradient: Gradient {
                        GradientStop {
                            color: "#4d5845"
                            position: 0.85
                        }

                        GradientStop {
                            color: "transparent"
                            position: 0.0
                        }
                    }
                }
            }
        }
    }
}

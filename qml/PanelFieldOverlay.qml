import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt

MouseArea {
    id: control

    property var field

    signal finished

    onClicked: control.finished()

    Rectangle {
        anchors.fill: parent
        color: "#000"
        opacity: 0.7
    }

    VRect {
        anchors.centerIn: parent
        height: control.height * 0.8
        width: control.width * 0.75

        MouseArea {
            anchors.fill: parent
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15

            VLabel {
                Layout.fillWidth: true
                font.bold: true
                font.italic: true
                text: "Select data to display:"
            }

            ListView {
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true
                model: control.field.depth > 0 ? panelState.collectors.filter(x => x.hasDbl) : panelState.collectors
                spacing: 10

                delegate: VBevelRect {
                    height: 100
                    style: modelData === control.field.collector ? 2 : 0
                    width: ListView.view.width

                    VLabel {
                        anchors.left: parent.left
                        anchors.margins: 15
                        anchors.top: parent.top
                        font.bold: true
                        font.italic: true
                        text: modelData.displayName
                    }

                    VLabel {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.margins: 15
                        anchors.right: parent.right
                        elide: Label.ElideRight
                        text: modelData.description
                    }

                    VLabel {
                        id: valueLabel

                        anchors.margins: 15
                        anchors.right: parent.right
                        anchors.top: parent.top

                        Component.onCompleted: valueLabel.text = modelData.getStr() || "N/A"
                    }

                    MouseArea {
                        anchors.fill: parent

                        onClicked: {
                            control.field.collector = modelData;
                            control.finished();
                        }
                    }
                }
                footer: Item {
                    height: 10
                }
                header: Item {
                    height: 10
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: 10

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
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 10

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

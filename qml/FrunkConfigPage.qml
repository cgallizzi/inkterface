import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt

Item {
    id: control

    property FrunkInfo frunk: frunkFinder.frunk

    signal error(var message)
    signal frunkCleared
    signal notification(var message)

    Settings {
        id: settings
    }

    RowLayout {
        id: topRow

        anchors.left: parent.left
        anchors.margins: 10
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: 10

        VLabel {
            id: nameLabel

            Layout.fillWidth: true
            elide: Label.ElideRight
            font.pixelSize: 36
            text: `${frunk.name}, ${frunk.rssi}, ${frunk.ifaceVersion}`
        }

        VConfirmButton {
            normalText: "Clear Frunk"

            onConfirmed: {
                settings.setValue("frunkName", "");
                control.frunkCleared();
            }
        }

        VConfirmButton {
            normalText: "Exit"

            onConfirmed: Qt.callLater(Qt.quit)
        }
    }

    GridLayout {
        id: readoutGrid

        anchors.bottom: serviceRect.top
        anchors.bottomMargin: 10
        anchors.left: topRow.left
        anchors.right: topRow.right
        anchors.top: topRow.bottom
        anchors.topMargin: 10
        columnSpacing: 10
        columns: 3
        rowSpacing: 10

        VBoxedReadout {
            Layout.fillWidth: true
            title: "--"
            value: "--"
        }

        VBoxedReadout {
            Layout.fillWidth: true
            title: "--"
            value: "--"
        }

        VBoxedReadout {
            Layout.fillWidth: true
            title: "--"
            value: "--"
        }

        VSparkline {
            Layout.fillHeight: true
            Layout.fillWidth: true
            title: "--"
            value: "--"
        }

        VSparkline {
            Layout.fillHeight: true
            Layout.fillWidth: true
            title: "--"
            value: "--"
        }

        VSparkline {
            Layout.fillHeight: true
            Layout.fillWidth: true
            title: "--"
            value: "--"
        }

        VSparkline {
            Layout.fillHeight: true
            Layout.fillWidth: true
            title: "--"
            value: "--"
        }

        VSparkline {
            Layout.fillHeight: true
            Layout.fillWidth: true
            title: "--"
            value: "--"
        }

        VSparkline {
            Layout.fillHeight: true
            Layout.fillWidth: true
            title: "--"
            value: "--"
        }
    }

    VLabel {
        anchors.centerIn: readoutGrid
        font.pixelSize: readoutGrid.height * 0.2
        rotation: 30
        text: "COMING SOON"
    }

    VBevelRect {
        id: serviceRect

        anchors.bottom: parent.bottom
        anchors.left: readoutGrid.left
        anchors.margins: 10
        anchors.right: parent.right
        anchors.rightMargin: -(lineWidth + 1)
        height: 72
        shape: 1
        style: svcMgr.isRunning ? 2 : 1

        RowLayout {
            anchors.left: parent.left
            anchors.leftMargin: 15
            anchors.right: parent.right
            anchors.rightMargin: Math.abs(parent.anchors.rightMargin) + 10
            anchors.top: parent.top
            anchors.topMargin: 10
            spacing: 10

            VLabel {
                font.bold: true
                font.italic: true
                text: "Service:"
            }

            VLabel {
                Layout.fillWidth: true
                elide: Label.ElideRight
                text: {
                    if (svcMgr.isRunning) {
                        return "Running";
                    } else if (svcMgr.isInstalled) {
                        return "Installed, Not Running";
                    }
                    return "Not Installed";
                }
            }

            VButton {
                enabled: svcMgr.isInstalled
                text: svcMgr.isRunning ? "Restart" : "Start"

                onClicked: svcMgr.isRunning ? svcMgr.restartService() : svcMgr.startService()
            }

            VButton {
                text: svcMgr.isInstalled ? "Uninstall" : "Install"

                onClicked: svcMgr.isInstalled ? svcMgr.uninstallService() : svcMgr.installService()
            }
        }
    }

            }
        }
    }
}

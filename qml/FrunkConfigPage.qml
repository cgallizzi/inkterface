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
            font.pixelSize: 36
            text: `${frunk.name}, ${frunk.rssi}, ${frunk.ifaceVersion}`
            elide: Label.ElideRight
            Layout.fillWidth: true
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
        columns: 3
        columnSpacing: 10
        rowSpacing: 10
        anchors.top: topRow.bottom
        anchors.left: topRow.left
        anchors.right: topRow.right
        anchors.bottom: serviceRect.top
        anchors.topMargin: 10
        anchors.bottomMargin: 10

        VBoxedReadout {
            title: "--"
            value: "--"
            Layout.fillWidth: true
        }

        VBoxedReadout {
            title: "--"
            value: "--"
            Layout.fillWidth: true
        }

        VBoxedReadout {
            title: "--"
            value: "--"
            Layout.fillWidth: true
        }

        VSparkline {
            title: "--"
            value: "--"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        VSparkline {
            title: "--"
            value: "--"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        VSparkline {
            title: "--"
            value: "--"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        VSparkline {
            title: "--"
            value: "--"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        VSparkline {
            title: "--"
            value: "--"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        VSparkline {
            title: "--"
            value: "--"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    VLabel {
        text: "COMING SOON"
        font.pixelSize: readoutGrid.height * 0.2
        rotation: 30
        anchors.centerIn: readoutGrid
    }

    VBevelRect {
        id: serviceRect
        shape: 1
        height: 72
        anchors.bottom: parent.bottom
        anchors.left: readoutGrid.left
        anchors.right: parent.right
        anchors.margins: 10
        anchors.rightMargin: -(lineWidth + 1)

        RowLayout {
            spacing: 10
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 10
            anchors.leftMargin: 15
            anchors.rightMargin: Math.abs(parent.anchors.rightMargin) + 10

            VLabel {
                text: "Service:"
                font.bold: true
                font.italic: true
            }

            VLabel {
                text: "Not Installed"
                elide: Label.ElideRight
                Layout.fillWidth: true
            }

            VButton {
                text: "Install"
            }
        }
    }
}

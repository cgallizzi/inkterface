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

    ColumnLayout {
        id: titleLineLayout
        anchors.left: topRow.left
        anchors.right: topRow.right
        anchors.top: topRow.bottom
        anchors.topMargin: 10

        VLabel {
            text: frunkState.topLine
            font.pixelSize: 32
            font.bold: true
        }

        VLabel {
            text: frunkState.midLine
            font.pixelSize: 24
        }

        VLabel {
            text: frunkState.botLine
            font.pixelSize: 24
        }
    }

    GridLayout {
        id: readoutGrid

        anchors.bottom: serviceRect.top
        anchors.bottomMargin: 10
        anchors.left: topRow.left
        anchors.right: topRow.right
        anchors.top: titleLineLayout.bottom
        anchors.topMargin: 10
        columnSpacing: 10
        columns: 3
        rowSpacing: 10

        Repeater {
            model: frunkState.fields.filter((x) => x.depth <= 0)

            delegate: VBoxedReadout {
                Layout.fillWidth: true
                title: modelData.key || "--"
                value: modelData.val || "--"

                MouseArea {
                    anchors.fill: parent
                    onClicked: overlayLoader.setSource("FrunkFieldOverlay.qml", {"field": modelData})
                }
            }
        }

        Repeater {
            model: frunkState.fields.filter((x) => x.depth > 0)

            delegate: VSparkline {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: modelData.key || "--"
                value: modelData.val || "--"
                maxPoints: modelData.depth

                Component.onCompleted: setPoints(modelData.points)

                Connections {
                    target: modelData

                    function onPointsChanged() {
                        setPoints(modelData.points)
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: overlayLoader.setSource("FrunkFieldOverlay.qml", {"field": modelData})
                }
            }
        }
    }

    VBevelRect {
        id: serviceRect

        anchors.bottom: parent.bottom
        anchors.left: readoutGrid.left
        anchors.margins: 10
        anchors.right: parent.right
        anchors.rightMargin: -(lineWidth * 2)
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

    Loader {
        id: overlayLoader
        anchors.fill: parent
    }

    Connections {
        target: overlayLoader.item

        function onFinished() {
            overlayLoader.sourceComponent = null
        }
    }
}

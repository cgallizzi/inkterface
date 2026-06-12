import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt

Item {
    id: control

    signal error(var message)
    signal notification(var message)
    signal panelSelected

    function selectPanel() {
        if (panelList.count == 0) {
            return;
        }
        var panel = panelList.model[panelList.currentIndex];
        if (!panel.supported) {
            control.error("Cannot select an unsupported panel!");
            return;
        }
        settings.setValue("panelName", panel.name);
        control.panelSelected();
    }

    Settings {
        id: settings

    }

    MouseArea {
        anchors.fill: parent

        onClicked: panelList.forceActiveFocus()
    }

    VLabel {
        id: titleLabel

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 20
        font.pixelSize: 36
        text: panelList.count === 0 ? "Searching For Panels" : "Select Your Panel"
    }

    ListView {
        id: panelList

        anchors.left: prevButton.right
        anchors.right: nextButton.left
        anchors.verticalCenter: prevButton.verticalCenter
        currentIndex: 0
        focus: true
        height: prevButton.height * 0.75
        highlightFollowsCurrentItem: true
        highlightMoveDuration: 100
        highlightRangeMode: ListView.StrictlyEnforceRange
        interactive: false
        keyNavigationEnabled: true
        // model: 10
        model: panelFinder.panels
        orientation: ListView.Horizontal
        preferredHighlightBegin: (width / 2) - (currentItem.width / 2)
        preferredHighlightEnd: (width / 2) + (currentItem.width / 2)
        spacing: 20

        delegate: PanelDelegate {
            height: ListView.view.height
            name: modelData.name || "NAME"
            rssi: `${modelData.rssi || -1} RSSI`
            supported: !!modelData.supported
            version: modelData.ifaceVersion || "VER"

            VButton {
                anchors.bottom: parent.top
                anchors.bottomMargin: 10
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.left: parent.left
                anchors.right: parent.right
                text: "Select"
                visible: panelList.currentIndex === index && modelData.supported

                onClicked: control.selectPanel()
            }
        }

        Component.onCompleted: {
            currentIndex = 0;
            forceActiveFocus();
        }
        Keys.onEnterPressed: event => {
                                 control.selectPanel();
                             }
        Keys.onReturnPressed: event => {
                                  control.selectPanel();
                              }
    }

    Rectangle {
        anchors.bottom: prevButton.bottom
        anchors.left: parent.left
        anchors.right: panelList.left
        anchors.rightMargin: -panelList.preferredHighlightBegin
        anchors.top: prevButton.top

        gradient: Gradient {
            orientation: Gradient.Horizontal

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
        anchors.bottom: nextButton.bottom
        anchors.left: panelList.right
        anchors.leftMargin: -(panelList.width - panelList.preferredHighlightEnd)
        anchors.right: parent.right
        anchors.top: nextButton.top

        gradient: Gradient {
            orientation: Gradient.Horizontal

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

    VButton {
        id: prevButton

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: titleLabel.bottom
        anchors.topMargin: 40
        enabled: panelList.currentIndex > 0
        font.pixelSize: 52
        text: "<"
        width: 60

        onClicked: {
            panelList.currentIndex = Math.max(panelList.currentIndex - 1, 0);
            panelList.forceActiveFocus();
        }
    }

    VButton {
        id: nextButton

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.top: titleLabel.bottom
        anchors.topMargin: 40
        enabled: panelList.currentIndex < panelList.count - 1
        font.pixelSize: 52
        text: ">"
        width: 60

        onClicked: {
            panelList.currentIndex = Math.min(panelList.currentIndex + 1, panelList.count - 1);
            panelList.forceActiveFocus();
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        height: width
        running: panelFinder.panels.length === 0
        width: Math.min(control.width, control.height) * 0.2
    }

    VConfirmButton {
        anchors.margins: 10
        anchors.right: parent.right
        anchors.top: parent.top
        normalText: "Exit"

        onConfirmed: Qt.callLater(Qt.quit)
    }
}

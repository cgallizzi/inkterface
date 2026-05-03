import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt

Item {
    id: control

    signal error(var message)
    signal frunkSelected
    signal notification(var message)

    function selectFrunk() {
        if (frunkList.count == 0) {
            return;
        }
        var frunk = frunkList.model[frunkList.currentIndex];
        if (!frunk.supported) {
            control.error("Cannot select an unsupported frunk!");
            return;
        }
        settings.setValue("frunkName", frunk.name);
        control.frunkSelected();
    }

    Settings {
        id: settings

    }

    MouseArea {
        anchors.fill: parent

        onClicked: frunkList.forceActiveFocus()
    }

    VLabel {
        id: titleLabel

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 20
        font.pixelSize: 36
        text: frunkList.count === 0 ? "Searching For Frunks" : "Select Your Frunk"
    }

    ListView {
        id: frunkList

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
        model: frunkFinder.frunks
        orientation: ListView.Horizontal
        preferredHighlightBegin: (width / 2) - (currentItem.width / 2)
        preferredHighlightEnd: (width / 2) + (currentItem.width / 2)
        spacing: 20

        delegate: FrunkPanel {
            height: ListView.view.height
            name: modelData.name || "NAME"
            rssi: `${modelData.rssi || -1} RSSI`
            supported: !!modelData.supported
            version: modelData.ifaceVersion || "VER"

            VButton {
                visible: frunkList.currentIndex === index && modelData.supported
                text: "Select"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.top
                anchors.margins: 10

                onClicked: control.selectFrunk()
            }
        }

        Component.onCompleted: {
            currentIndex = 0;
            forceActiveFocus();
        }
        Keys.onEnterPressed: event => {
                                 control.selectFrunk();
                             }
        Keys.onReturnPressed: event => {
                                  control.selectFrunk();
                              }
    }

    Rectangle {
        anchors.bottom: prevButton.bottom
        anchors.left: parent.left
        anchors.right: frunkList.left
        anchors.rightMargin: -frunkList.preferredHighlightBegin
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
        anchors.left: frunkList.right
        anchors.leftMargin: -(frunkList.width - frunkList.preferredHighlightEnd)
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
        enabled: frunkList.currentIndex > 0
        font.pixelSize: 52
        text: "<"
        width: 60

        onClicked: {
            frunkList.currentIndex = Math.max(frunkList.currentIndex - 1, 0)
            frunkList.forceActiveFocus()
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
        enabled: frunkList.currentIndex < frunkList.count - 1
        font.pixelSize: 52
        text: ">"
        width: 60

        onClicked: {
            frunkList.currentIndex = Math.min(frunkList.currentIndex + 1, frunkList.count - 1)
            frunkList.forceActiveFocus()
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        height: width
        running: frunkFinder.frunks.length === 0
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

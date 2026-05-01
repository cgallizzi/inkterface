import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vqt

Item {
    id: control

    VLabel {
        id: titleLabel
        text: `Select a frunk to use with this machine: (${frunkList.currentIndex})`
        anchors.left: prevButton.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20
    }

    ListView {
        id: frunkList
        height: prevButton.height * 0.8
        anchors.verticalCenter: prevButton.verticalCenter
        anchors.left: prevButton.right
        anchors.right: nextButton.left
        spacing: 20
        orientation: ListView.Horizontal
        keyNavigationEnabled: true
        highlightFollowsCurrentItem: true
        highlightMoveDuration: 200
        highlightRangeMode: ListView.StrictlyEnforceRange
        preferredHighlightBegin: (width / 2) - (currentItem.width / 2)
        preferredHighlightEnd: (width / 2) + (currentItem.width / 2)
        currentIndex: 0

        model: frunkFinder.frunks
        // model: 10

        delegate: FrunkPanel {
            name: modelData.name
            rssi: `${modelData.rssi} RSSI`
            version: modelData.ifaceVersion
            supported: modelData.supported
            // name: "NAME"
            // rssi: "RSSI"
            // version: "VERSION"
            // opacity: !!modelData.supported ? 1.0 : 0.2
            height: ListView.view.height
        }
    }

    Rectangle {
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.25; color: "#4d5845" }
            GradientStop { position: 1.0; color: "transparent" }
        }
        anchors.left: parent.left
        anchors.top: prevButton.top
        anchors.bottom: prevButton.bottom
        anchors.right: frunkList.left
        anchors.rightMargin: -frunkList.preferredHighlightBegin
    }

    Rectangle {
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.75; color: "#4d5845" }
            GradientStop { position: 0.0; color: "transparent" }
        }
        anchors.right: parent.right
        anchors.top: nextButton.top
        anchors.bottom: nextButton.bottom
        anchors.left: frunkList.right
        anchors.leftMargin: -(frunkList.width - frunkList.preferredHighlightEnd)
    }

    VButton {
        id: prevButton
        text: "<"
        width: 50
        anchors.left: parent.left
        anchors.top: titleLabel.bottom
        anchors.bottom: parent.bottom
        anchors.margins: 40
        anchors.leftMargin: 20

        onClicked: frunkList.currentIndex = Math.max(frunkList.currentIndex - 1, 0)
    }

    VButton {
        id: nextButton
        text: ">"
        width: 50
        anchors.right: parent.right
        anchors.top: titleLabel.bottom
        anchors.bottom: parent.bottom
        anchors.margins: 40
        anchors.rightMargin: 20

        onClicked: frunkList.currentIndex = Math.min(frunkList.currentIndex + 1, frunkList.count - 1)
    }

    BusyIndicator {
        width: Math.min(control.width, control.height) * 0.2
        height: width
        anchors.centerIn: parent
        running: frunkFinder.frunks.length === 0
    }
}

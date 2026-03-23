import QtQuick

Rectangle {
    id: control
    color: control.bgColor

    // let users choose between normal, danger, alternate, etc.
    property int style: 0
    readonly property int _style: style % 3

    property int depth: 3
    property bool flipped: false
    property color fgColor: "#c2b05a" // yellowish light (text)
    property color hiColor: ["#697464", "#9d391a", "#1a6e9d"][control._style] // light desat green/red (highlight)
    property color bgColor: ["#4d5845", "#7f2e15", "#154c7f"][control._style] // dark green/red (moderate bg)
    property color dbgColor: ["#3e4638", "#662511", "#16446f"][control._style] // pretty dark green/red (dark bg)
    property color teColor: ["#7d8576", "#b74623", "#2382b7"][control._style] // very light desat green/red/gray
    property color reColor: ["#242f1f", "#42170a", "#0f385d"][control._style] // very dark green/red/near black
    property color leColor: control.hiColor
    property color beColor: control.reColor

    Rectangle {
        // left edge
        color: control.flipped ? control.reColor : control.leColor
        width: control.depth
        height: parent.height
    }

    Rectangle {
        // top edge
        color: control.flipped ? control.beColor : control.teColor
        width: parent.width
        height: control.depth
    }

    Rectangle {
        // right edge
        x: parent.width - width
        color: control.flipped ? control.leColor : control.reColor
        width: control.depth
        height: parent.height
    }

    Rectangle {
        // bottom edge
        y: parent.height - height
        color: control.flipped ? control.teColor : control.beColor
        width: parent.width
        height: control.depth
    }
}

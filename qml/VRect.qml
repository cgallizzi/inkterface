import QtQuick

Rectangle {
    id: control

    readonly property int _style: style % 3
    property color beColor: control.reColor
    property color bgColor: ["#4d5845", "#7f2e15", "#154c7f"][control._style] // dark green/red (moderate bg)
    property color dbgColor: ["#3e4638", "#662511", "#16446f"][control._style] // pretty dark green/red (dark bg)
    property int depth: 3
    property color fgColor: "#c2b05a" // yellowish light (text)
    property bool flipped: false
    property color hiColor: ["#697464", "#9d391a", "#1a6e9d"][control._style] // light desat green/red (highlight)
    property color leColor: control.hiColor
    property color reColor: ["#242f1f", "#42170a", "#0f385d"][control._style] // very dark green/red/near black

    // let users choose between normal, danger, alternate, etc.
    property int style: 0
    property color teColor: ["#7d8576", "#b74623", "#2382b7"][control._style] // very light desat green/red/gray

    color: control.flipped ? control.dbgColor : control.bgColor

    Rectangle {
        // left edge
        color: control.flipped ? control.reColor : control.leColor
        height: parent.height
        width: control.depth
    }

    Rectangle {
        // top edge
        color: control.flipped ? control.beColor : control.teColor
        height: control.depth
        width: parent.width
    }

    Rectangle {
        color: control.flipped ? control.leColor : control.reColor
        height: parent.height
        width: control.depth
        // right edge
        x: parent.width - width
    }

    Rectangle {
        color: control.flipped ? control.teColor : control.beColor
        height: control.depth
        width: parent.width
        // bottom edge
        y: parent.height - height
    }
}

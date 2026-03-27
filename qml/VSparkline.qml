import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import vqt

VRect {
    id: control
    flipped: true
    implicitWidth: 265
    implicitHeight: 110

    property int maxPoints: 15
    property alias font: titleLabel.font
    property color lineColor: control.fgColor
    property alias title: titleLabel.text
    property string value

    function setPoints(points) {
        var slcPoints = points.slice(-control.maxPoints)
        if (slcPoints.length <= 0) {
            return
        }
        var adjPoints = []
        var xMax = -Infinity
        var xMin = Infinity
        var yMax = -Infinity
        var yMin = Infinity
        for (const p of slcPoints) {
            xMax = Math.max(p.x, xMax)
            xMin = Math.min(p.x, xMin)
            yMax = Math.max(p.y, yMax)
            yMin = Math.min(p.y, yMin)
        }
        if (yMax === yMin) {
            yMax = yMax + 1
            yMin = yMin - 1
        }
        for (const p of slcPoints) {
            adjPoints.push(Qt.point(p.x - xMin, -(p.y - yMax)))
        }
        path.range = Qt.point(
            Math.max(1, Math.abs(xMax - xMin)),
            Math.max(1, Math.abs(yMax - yMin))
        )
        polyline.path = adjPoints
        control.value = formatPoint(slcPoints[slcPoints.length - 1])
    }

    // override this as desired
    function formatPoint(p) {
        return p.y
    }

    VLabel {
        id: titleLabel
        color: control.lineColor
        elide: Label.ElideRight
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: valueLabel.left
        anchors.topMargin: parent.depth
        anchors.leftMargin: parent.depth + 5
        anchors.rightMargin: 10
    }

    VLabel {
        id: valueLabel
        text: `<i><b>${control.value}</b></i>`
        color: titleLabel.color
        font: titleLabel.font
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: parent.depth
        anchors.rightMargin: parent.depth + 5
    }

    Rectangle {
        id: divider
        color: control.lineColor
        opacity: 0.5
        height: 2
        anchors.top: titleLabel.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: parent.depth
        anchors.topMargin: 5
    }

    Shape {
        id: shape
        anchors.top: divider.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: parent.depth + 10
        anchors.topMargin: 10

        ShapePath {
            id: path
            strokeWidth: 4
            strokeColor: control.lineColor
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            scale: Qt.size(shape.width / path.range.x, shape.height / path.range.y)

            property point range: Qt.point(1, 1)

            PathPolyline {
                id: polyline
            }
        }
    }
}

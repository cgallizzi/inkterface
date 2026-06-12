import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import vqt

VRect {
    id: control

    property alias font: titleLabel.font
    property color lineColor: control.fgColor
    property int maxPoints: 15
    property alias title: titleLabel.text
    property bool autoValue: true
    property string value

    // override this as desired
    function formatPoint(p) {
        return p.y;
    }

    function setPoints(points) {
        var slcPoints = points.slice(-control.maxPoints);
        if (slcPoints.length <= 0) {
            return;
        }
        var adjPoints = [];
        var xMax = -Infinity;
        var xMin = Infinity;
        var yMax = -Infinity;
        var yMin = Infinity;
        for (const p of slcPoints) {
            xMax = Math.max(p.x, xMax);
            xMin = Math.min(p.x, xMin);
            yMax = Math.max(p.y, yMax);
            yMin = Math.min(p.y, yMin);
        }
        if (yMax === yMin) {
            yMax = yMax + 1;
            yMin = yMin - 1;
        }
        for (const p of slcPoints) {
            adjPoints.push(Qt.point(p.x - xMin, -(p.y - yMax)));
        }
        path.range = Qt.point(Math.max(1, Math.abs(xMax - xMin)), Math.max(1, Math.abs(yMax - yMin)));
        polyline.path = adjPoints;
        if (control.autoValue) {
            control.value = formatPoint(slcPoints[slcPoints.length - 1]);
        }
    }

    flipped: true
    implicitHeight: 110
    implicitWidth: 265

    VLabel {
        id: titleLabel

        anchors.left: parent.left
        anchors.leftMargin: parent.depth + 5
        anchors.right: valueLabel.left
        anchors.rightMargin: 10
        anchors.top: parent.top
        anchors.topMargin: parent.depth
        color: control.lineColor
        elide: Label.ElideRight
    }

    VLabel {
        id: valueLabel

        anchors.right: parent.right
        anchors.rightMargin: parent.depth + 5
        anchors.top: parent.top
        anchors.topMargin: parent.depth
        color: titleLabel.color
        font: titleLabel.font
        text: `<i><b>${control.value}</b></i>`
    }

    Rectangle {
        id: divider

        anchors.left: parent.left
        anchors.margins: parent.depth
        anchors.right: parent.right
        anchors.top: titleLabel.bottom
        anchors.topMargin: 5
        color: control.lineColor
        height: 2
        opacity: 0.5
    }

    Shape {
        id: shape

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: parent.depth + 10
        anchors.right: parent.right
        anchors.top: divider.bottom
        anchors.topMargin: 10

        ShapePath {
            id: path

            property point range: Qt.point(1, 1)

            capStyle: ShapePath.RoundCap
            fillColor: "transparent"
            scale: Qt.size(shape.width / path.range.x, shape.height / path.range.y)
            strokeColor: control.lineColor
            strokeWidth: 4

            PathPolyline {
                id: polyline

            }
        }
    }
}

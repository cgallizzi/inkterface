import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import vqt

Item {
    id: control
    implicitHeight: 100
    implicitWidth: 200


    property int bevelSize: 10
    property color lineColor: ["#c2b05a", "#b74623", "#2382b7"][control._style]
    property alias lineWidth: path.strokeWidth
    property int shape: 0
    readonly property int _shape: control.shape % 3
    property int style: 0
    readonly property int _style: control.style % 3

    readonly property var shapes: {
        // all sides
        0: [ Qt.point(bevelSize, 0) // top left
           , Qt.point(shape.width - bevelSize, 0) // top right
           , Qt.point(shape.width, bevelSize) // right top
           , Qt.point(shape.width, shape.height - bevelSize) // right bottom
           , Qt.point(shape.width - bevelSize, shape.height) // bottom right
           , Qt.point(bevelSize, shape.height) // bottom left
           , Qt.point(0, shape.height - bevelSize) // left bottom
           , Qt.point(0, bevelSize) // left top
           , Qt.point(bevelSize, 0) // top left (to close shape)
           ],
        // left side
        1: [ Qt.point(bevelSize, 0) // top left
           , Qt.point(shape.width, 0) // top right
           , Qt.point(shape.width, shape.height) // bottom right
           , Qt.point(bevelSize, shape.height) // bottom left
           , Qt.point(0, shape.height - bevelSize) // left bottom
           , Qt.point(0, bevelSize) // left top
           , Qt.point(bevelSize, 0) // top left (to close shape)
           ],
        // right side
        2: [ Qt.point(0, 0) // top left
           , Qt.point(shape.width - bevelSize, 0) // top right
           , Qt.point(shape.width, bevelSize) // right top
           , Qt.point(shape.width, shape.height - bevelSize) // right bottom
           , Qt.point(shape.width - bevelSize, shape.height) // bottom right
           , Qt.point(0, shape.height) // bottom left
           , Qt.point(0, 0) // top left (to close shape)
           ],
    }

    Shape {
        id: shape

        anchors.fill: parent
        anchors.margins: control.lineWidth / 2

        ShapePath {
            id: path

            capStyle: ShapePath.FlatCap
            fillColor: "transparent"
            strokeColor: control.lineColor
            strokeWidth: 4

            PathPolyline {
                id: polyline

                path: control.shapes[control._shape]
            }
        }
    }
}

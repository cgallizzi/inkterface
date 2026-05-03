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
           , Qt.point(control.width - bevelSize, 0) // top right
           , Qt.point(control.width, bevelSize) // right top
           , Qt.point(control.width, control.height - bevelSize) // right bottom
           , Qt.point(control.width - bevelSize, control.height) // bottom right
           , Qt.point(bevelSize, control.height) // bottom left
           , Qt.point(0, control.height - bevelSize) // left bottom
           , Qt.point(0, bevelSize) // left top
           , Qt.point(bevelSize, 0) // top left (to close shape)
           ],
        // left side
        1: [ Qt.point(bevelSize, 0) // top left
           , Qt.point(control.width, 0) // top right
           , Qt.point(control.width, control.height) // bottom right
           , Qt.point(bevelSize, control.height) // bottom left
           , Qt.point(0, control.height - bevelSize) // left bottom
           , Qt.point(0, bevelSize) // left top
           , Qt.point(bevelSize, 0) // top left (to close shape)
           ],
        // right side
        2: [ Qt.point(0, 0) // top left
           , Qt.point(control.width - bevelSize, 0) // top right
           , Qt.point(control.width, bevelSize) // right top
           , Qt.point(control.width, control.height - bevelSize) // right bottom
           , Qt.point(control.width - bevelSize, control.height) // bottom right
           , Qt.point(0, control.height) // bottom left
           , Qt.point(0, 0) // top left (to close shape)
           ],
    }

    Shape {
        id: shape

        anchors.fill: parent

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

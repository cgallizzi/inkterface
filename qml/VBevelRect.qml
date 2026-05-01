import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import vqt

Item {
    id: control

    property int bevelSize: 10
    property alias lineColor: path.strokeColor
    property alias lineWidth: path.strokeWidth

    Shape {
        id: shape

        anchors.fill: parent

        ShapePath {
            id: path

            capStyle: ShapePath.FlatCap
            fillColor: "transparent"
            strokeColor: "#c2b05a"
            strokeWidth: 4

            PathPolyline {
                id: polyline

                path: [Qt.point(bevelSize, 0) // top left
                    , Qt.point(control.width - bevelSize, 0) // top right
                    , Qt.point(control.width, bevelSize) // right top
                    , Qt.point(control.width, control.height - bevelSize) // right bottom
                    , Qt.point(control.width - bevelSize, control.height) // bottom right
                    , Qt.point(bevelSize, control.height) // bottom left
                    , Qt.point(0, control.height - bevelSize) // left bottom
                    , Qt.point(0, bevelSize) // left top
                    , Qt.point(bevelSize, 0) // top left (to close shape)
                    ,]
            }
        }
    }
}

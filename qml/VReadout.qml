import QtQuick
import QtQuick.Controls
import vqt

VLabel {
    id: control

    property string name
    property string value

    text: `${control.name}:<br>&nbsp;<b><i>${control.value}</i></b>`
}

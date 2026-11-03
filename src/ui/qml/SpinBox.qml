import QtQuick
import QtQuick.Controls as Controls

Controls.SpinBox {
    id: control

    implicitHeight: 36
    font.family: Theme.readerFont
    font.pixelSize: 13

    palette.text: Theme.textPrimary
    palette.base: Theme.surface
    palette.button: Theme.surfaceMuted
    palette.buttonText: Theme.textPrimary

    background: Rectangle {
        radius: 9
        color: Theme.surface
        border.color: control.activeFocus ? Theme.accent : Theme.border
        border.width: control.activeFocus ? 2 : 1
    }
}

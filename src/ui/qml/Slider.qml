import QtQuick
import QtQuick.Controls as Controls

Controls.Slider {
    id: control

    implicitHeight: 28

    background: Rectangle {
        x: control.leftPadding
        y: (control.height - height) / 2
        width: control.availableWidth
        height: 6
        radius: 3
        color: Theme.surfaceMuted
        border.color: Theme.border

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: 3
            color: Theme.accent
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * control.availableWidth - width / 2
        y: (control.height - height) / 2
        width: 16
        height: 16
        radius: 8
        color: control.pressed ? Theme.accentPressed : Theme.accent
        border.color: control.activeFocus ? Theme.textPrimary : Theme.accentSoft
        border.width: control.activeFocus ? 2 : 1
    }
}

import QtQuick
import QtQuick.Controls as Controls

Controls.TextField {
    id: control

    implicitHeight: 36
    leftPadding: 14
    rightPadding: 14
    topPadding: 6
    bottomPadding: 6
    font.family: Theme.readerFont
    font.pixelSize: 13
    color: Theme.textPrimary
    placeholderTextColor: Theme.textSecondary
    selectionColor: Theme.accent
    selectedTextColor: "#ffffff"

    background: Rectangle {
        radius: 10
        color: control.activeFocus ? Theme.surfaceRaised : Theme.surface
        border.color: control.activeFocus ? Theme.accent : Theme.border
        border.width: control.activeFocus ? 2 : 1

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
        Behavior on border.color {
            ColorAnimation { duration: 120 }
        }
    }
}

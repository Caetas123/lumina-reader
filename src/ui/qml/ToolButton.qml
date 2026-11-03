import QtQuick
import QtQuick.Controls as Controls

Controls.ToolButton {
    id: control

    implicitHeight: 34
    implicitWidth: Math.max(32, contentItem.implicitWidth + leftPadding + rightPadding)
    leftPadding: 8
    rightPadding: 8
    topPadding: 6
    bottomPadding: 6
    font.family: Theme.readerFont
    font.pixelSize: 13
    font.weight: Font.Medium

    contentItem: Text {
        text: control.text
        font: control.font
        color: !control.enabled
            ? Theme.textSecondary
            : control.checked ? Theme.accent : Theme.textPrimary
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        opacity: control.enabled ? 1.0 : 0.58
    }

    background: Rectangle {
        radius: 8
        color: control.checked
            ? Theme.accentSoft
            : control.pressed
                ? Theme.surface
                : control.hovered ? Theme.surfaceRaised : "transparent"
        border.color: control.activeFocus
            ? Theme.accent
            : control.hovered || control.checked ? Theme.border : "transparent"
        border.width: control.activeFocus ? 2 : 1

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
    }
}

import QtQuick
import QtQuick.Controls as Controls

Controls.CheckBox {
    id: control

    implicitHeight: 28
    spacing: 9
    font.family: Theme.readerFont
    font.pixelSize: 13

    indicator: Rectangle {
        x: 0
        y: (control.height - height) / 2
        width: 20
        height: 20
        radius: 6
        color: control.checked ? Theme.accent : "transparent"
        border.color: control.checked ? Theme.accent : Theme.border
        border.width: control.activeFocus ? 2 : 1

        Text {
            anchors.centerIn: parent
            text: "✓"
            visible: control.checked
            color: "#ffffff"
            font.pixelSize: 13
            font.weight: Font.Bold
        }
    }

    contentItem: Text {
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        color: control.enabled ? Theme.textPrimary : Theme.textSecondary
        font: control.font
        verticalAlignment: Text.AlignVCenter
        opacity: control.enabled ? 1.0 : 0.62
    }
}

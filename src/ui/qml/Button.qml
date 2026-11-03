import QtQuick
import QtQuick.Controls as Controls

Controls.Button {
    id: control

    implicitHeight: 36
    implicitWidth: Math.max(88, contentItem.implicitWidth + leftPadding + rightPadding)
    property string iconName: ""
    property color iconColor: control.enabled ? Theme.textPrimary : Theme.textSecondary
    leftPadding: 12
    rightPadding: 12
    topPadding: 6
    bottomPadding: 6
    spacing: 8
    font.family: Theme.readerFont
    font.pixelSize: 12
    font.weight: Font.Medium

    contentItem: Item {
        implicitWidth: buttonRow.implicitWidth
        implicitHeight: buttonRow.implicitHeight

        Row {
            id: buttonRow
            anchors.centerIn: parent
            spacing: control.iconName.length > 0 ? 8 : 0

            Icon {
                visible: control.iconName.length > 0
                name: control.iconName
                iconColor: !control.enabled
                    ? Theme.textSecondary
                    : control.highlighted || (control.checkable && control.checked)
                        ? "#ffffff" : control.iconColor
                width: 16
                height: 16
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: control.text
                font: control.font
                color: !control.enabled
                    ? Theme.textSecondary
                    : control.highlighted || (control.checkable && control.checked)
                        ? "#ffffff"
                        : Theme.textPrimary
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                opacity: control.enabled ? 1.0 : 0.62
            }
        }
    }

    background: Rectangle {
        implicitHeight: 36
        radius: 9
        color: {
            if (!control.enabled) {
                return Theme.surfaceMuted
            }
            if (control.checkable) {
                return control.checked
                    ? Theme.accent
                    : control.hovered ? Theme.surfaceRaised : "transparent"
            }
            if (control.highlighted) {
                return control.pressed
                    ? Theme.accentPressed
                    : control.hovered ? Qt.lighter(Theme.accent, 1.12) : Theme.accent
            }
            return control.pressed
                ? Theme.surface
                : control.hovered ? Theme.surfaceRaised : Theme.surfaceMuted
        }
        border.color: {
            if (!control.enabled) {
                return Theme.border
            }
            if (control.highlighted || (control.checkable && control.checked)) {
                return control.activeFocus || control.hovered ? Theme.accentSoft : Theme.accent
            }
            return control.activeFocus
                ? Theme.accent
                : control.checkable ? "transparent" : Theme.border
        }
        border.width: control.activeFocus ? 2 : 1

        Rectangle {
            anchors.fill: parent
            visible: control.enabled && control.highlighted
            radius: parent.radius
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#7f63f4" }
                GradientStop { position: 1.0; color: "#a36cff" }
            }
            opacity: control.pressed ? 0.82 : control.hovered ? 1.0 : 0.94
        }

        Rectangle {
            visible: control.checkable && control.checked
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 3
            radius: 2
            color: Theme.accent
        }

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
        Behavior on border.color {
            ColorAnimation { duration: 120 }
        }
    }
}

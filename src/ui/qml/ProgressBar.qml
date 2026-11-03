import QtQuick
import QtQuick.Controls as Controls

Controls.ProgressBar {
    id: control

    implicitHeight: 8

    background: Rectangle {
        implicitHeight: 8
        radius: 4
        color: Theme.surfaceMuted
        border.color: Theme.border
    }

    contentItem: Item {
        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: 4
            color: Theme.accent
        }
    }
}

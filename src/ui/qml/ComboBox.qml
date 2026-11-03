pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls

Controls.ComboBox {
    id: control

    implicitHeight: 36
    leftPadding: 14
    rightPadding: 38
    topPadding: 6
    bottomPadding: 6
    font.family: Theme.readerFont
    font.pixelSize: 13

    contentItem: Text {
        text: control.displayText
        font: control.font
        color: control.enabled ? Theme.textPrimary : Theme.textSecondary
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        opacity: control.enabled ? 1.0 : 0.62
    }

    indicator: Canvas {
        id: indicatorCanvas
        x: control.width - width - 14
        y: (control.height - height) / 2
        width: 12
        height: 8

        onPaint: {
            var context = getContext("2d")
            context.reset()
            context.strokeStyle = control.enabled ? Theme.textSecondary : Theme.border
            context.lineWidth = 2
            context.lineCap = "round"
            context.beginPath()
            context.moveTo(1, 2)
            context.lineTo(6, 6)
            context.lineTo(11, 2)
            context.stroke()
        }

        Connections {
            target: Theme
            function onModeChanged() { indicatorCanvas.requestPaint() }
        }
    }

    background: Rectangle {
        radius: 10
        color: control.activeFocus || control.popup.visible ? Theme.surfaceRaised : Theme.surface
        border.color: control.activeFocus || control.popup.visible ? Theme.accent : Theme.border
        border.width: control.activeFocus ? 2 : 1
    }

    popup: Controls.Popup {
        y: control.height - 1
        width: control.width
        padding: 5
        topMargin: 4
        bottomMargin: 4

        contentItem: ListView {
            clip: true
            implicitHeight: Math.min(contentHeight, 280)
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
        }

        background: Rectangle {
            radius: 10
            color: Theme.surfaceRaised
            border.color: Theme.border
            border.width: 1
        }
    }

    delegate: Controls.ItemDelegate {
        id: delegateItem
        width: control.width - control.popup.leftPadding - control.popup.rightPadding
        height: 36
        padding: 10
        highlighted: control.highlightedIndex === index

        contentItem: Text {
            text: control.textRole.length > 0 ? model[control.textRole] : modelData
            color: Theme.textPrimary
            font.family: Theme.readerFont
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: 7
            color: delegateItem.highlighted ? Theme.accentSoft : "transparent"
        }
    }
}

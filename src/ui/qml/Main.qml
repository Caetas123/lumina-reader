import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import LuminaReader 1.0

ApplicationWindow {
    id: window

    visible: true
    width: 1100
    height: 720
    minimumWidth: 720
    minimumHeight: 480
    title: appController.applicationName
    color: Theme.windowBackground

    header: ToolBar {
        contentHeight: 56
        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 12

            Label {
                text: qsTr("Lumina Reader")
                color: Theme.textPrimary
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }

            Label {
                text: qsTr("Foundation")
                color: Theme.textSecondary
                font.pixelSize: 14
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("v%1").arg(appController.applicationVersion)
                color: Theme.textSecondary
                font.pixelSize: 13
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 620)
        spacing: 18

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("A calm, local-first PDF reading experience")
            color: Theme.textPrimary
            font.pixelSize: 30
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("The project foundation is ready. Library and reader modules will be added incrementally.")
            color: Theme.textSecondary
            font.pixelSize: 16
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 92
            radius: 12
            color: Theme.surface
            border.color: Theme.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 6

                Label {
                    text: qsTr("Phase 0 — Foundation")
                    color: Theme.textPrimary
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Qt Quick, CMake, a base theme, and a test target are configured.")
                    color: Theme.textSecondary
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                }
            }
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Quit")
            onClicked: Qt.quit()
        }
    }
}

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

            Image {
                source: "qrc:/qt/qml/LuminaReader/assets/lumina-logo.png"
                sourceSize.width: 36
                sourceSize.height: 36
                fillMode: Image.PreserveAspectFit
                smooth: true
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
            }

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
            implicitHeight: appController.databaseReady || appController.startupError.length === 0 ? 92 : 132
            radius: 12
            color: appController.databaseReady ? "#ecfdf5" : Theme.surface
            border.color: appController.databaseReady ? "#a7f3d0" : Theme.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 6

                Label {
                    text: appController.databaseReady
                        ? qsTr("Local database ready")
                        : qsTr("Phase 0 — Foundation")
                    color: appController.databaseReady ? Theme.success : Theme.textPrimary
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    text: appController.databaseReady
                        ? qsTr("Your application data stays on this device by default.")
                        : qsTr("Qt Quick, CMake, a base theme, and a test target are configured.")
                    color: Theme.textSecondary
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                }

                Label {
                    Layout.fillWidth: true
                    visible: !appController.databaseReady && appController.startupError.length > 0
                    text: appController.startupError
                    color: Theme.warning
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }
            }
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            visible: !appController.databaseReady && appController.startupError.length > 0
            text: qsTr("Retry database initialization")
            onClicked: appController.retryInitialization()
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Quit")
            onClicked: Qt.quit()
        }
    }
}

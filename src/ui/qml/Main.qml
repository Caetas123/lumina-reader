import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import LuminaReader 1.0

ApplicationWindow {
    id: window

    visible: true
    width: 1280
    height: 800
    minimumWidth: 980
    minimumHeight: 620
    title: appController.applicationName
    color: Theme.windowBackground

    property int currentView: 0
    property string notification: ""
    property var themeKeys: ["light", "dark", "sepia", "paper", "gray", "high-contrast", "low-light"]

    function applyTheme() {
        Theme.mode = appController.settings.highContrast ? "high-contrast" : appController.settings.theme
    }

    Component.onCompleted: applyTheme()

    Connections {
        target: appController.settings
        function onSettingsChanged() {
            window.applyTheme()
        }
    }

    function localPath(url) {
        var path = decodeURIComponent(String(url).replace(/^file:\/\//, ""))
        if (Qt.platform.os === "windows" && path.startsWith("/") && path.length > 2 && path.charAt(2) === ":") {
            path = path.substring(1)
        }
        return path
    }

    function openImportedFiles(urls) {
        var paths = []
        for (var index = 0; index < urls.length; ++index) {
            paths.push(localPath(urls[index]))
        }
        if (paths.length > 0 && appController.library.importFiles(paths)) {
            notification = qsTr("Added %1 document(s) to your library.").arg(paths.length)
            notificationTimer.restart()
        }
    }

    function fileUrl(path) {
        if (!path || path.length === 0) {
            return ""
        }
        if (String(path).startsWith("file:")) {
            return path
        }
        var normalized = String(path).replace(/\\/g, "/")
        return normalized.startsWith("/") ? "file://" + normalized : "file:///" + normalized
    }

    function setLibraryFilter(filter) {
        appController.library.viewFilter = filter
        window.currentView = 0
    }

    function openMetadata(documentId) {
        var record = appController.library.documentDetails(documentId)
        if (!record || !record.id) {
            return
        }
        metadataDialog.documentId = documentId
        metadataTitle.text = record.title || ""
        metadataAuthor.text = record.author || ""
        metadataLanguage.text = record.language || ""
        metadataDescription.text = record.description || ""
        metadataTags.text = appController.library.tagsForDocument(documentId).join(", ")
        metadataDialog.open()
    }

    function statusLabel(status) {
        switch (status) {
        case "reading": return qsTr("Reading")
        case "read": return qsTr("Finished")
        case "abandoned": return qsTr("Abandoned")
        default: return qsTr("To read")
        }
    }

    Timer {
        id: notificationTimer
        interval: 3600
        onTriggered: window.notification = ""
    }

    FileDialog {
        id: importDialog
        title: qsTr("Add documents to Lumina Reader")
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            qsTr("Supported documents (%1)").arg(appController.library.supportedExtensions().join(" ")),
            qsTr("All files (*)")
        ]
        onAccepted: window.openImportedFiles(selectedFiles)
    }

    FolderDialog {
        id: importFolderDialog
        title: qsTr("Add a folder to Lumina Reader")
        onAccepted: {
            var path = window.localPath(selectedFolder)
            if (appController.library.importFolder(path, true)) {
                window.notification = qsTr("The folder was added to your library.")
            } else {
                window.notification = qsTr("No supported documents were found in that folder.")
            }
            notificationTimer.restart()
        }
    }

    Dialog {
        id: metadataDialog
        property string documentId: ""
        modal: true
        title: qsTr("Edit document details")
        width: Math.min(window.width - 80, 560)
        standardButtons: Dialog.Ok | Dialog.Cancel

        contentItem: ColumnLayout {
            spacing: 12

            Label {
                text: qsTr("These details are stored in your local library and do not change the original file.")
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            TextField {
                id: metadataTitle
                Layout.fillWidth: true
                placeholderText: qsTr("Title")
            }
            TextField {
                id: metadataAuthor
                Layout.fillWidth: true
                placeholderText: qsTr("Author")
            }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: metadataLanguage
                    Layout.fillWidth: true
                    placeholderText: qsTr("Language")
                }
                TextField {
                    id: metadataTags
                    Layout.fillWidth: true
                    placeholderText: qsTr("Tags, separated by commas")
                }
            }
            TextArea {
                id: metadataDescription
                Layout.fillWidth: true
                Layout.preferredHeight: 110
                placeholderText: qsTr("Personal description")
                wrapMode: TextArea.Wrap
            }
        }

        onAccepted: {
            var saved = appController.library.updateMetadata(documentId,
                                                              metadataTitle.text,
                                                              metadataAuthor.text,
                                                              metadataDescription.text,
                                                              metadataLanguage.text)
            if (saved) {
                var tags = metadataTags.text.split(",")
                saved = appController.library.setTags(documentId, tags)
            }
            appController.library.refresh()
            window.notification = saved ? qsTr("Document details saved.") : qsTr("The document details could not be saved.")
            notificationTimer.restart()
        }
    }

    FileDialog {
        id: knowledgeExportDialog
        title: qsTr("Export knowledge")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "md"
        nameFilters: [qsTr("Markdown (*.md)"), qsTr("JSON (*.json)"), qsTr("Plain text (*.txt)")]
        onAccepted: {
            var path = window.localPath(selectedFile)
            var lower = path.toLower()
            var format = lower.endsWith(".json") ? "json" : lower.endsWith(".txt") ? "txt" : "markdown"
            if (appController.backup.exportKnowledge(path, format)) {
                window.notification = qsTr("Knowledge exported successfully.")
            } else {
                window.notification = appController.backup.lastError
            }
            notificationTimer.restart()
        }
    }

    FileDialog {
        id: libraryExportDialog
        title: qsTr("Export library")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("Lumina library JSON (*.json)")]
        onAccepted: {
            var path = window.localPath(selectedFile)
            window.notification = appController.backup.exportLibrary(path)
                ? qsTr("Library exported successfully.") : appController.backup.lastError
            notificationTimer.restart()
        }
    }

    FileDialog {
        id: libraryRestoreDialog
        title: qsTr("Restore a Lumina library export")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Lumina library JSON (*.json)"), qsTr("All files (*)")]
        onAccepted: {
            var path = window.localPath(selectedFile)
            if (appController.backup.restoreLibrary(path)) {
                appController.library.refresh()
                window.notification = qsTr("Library metadata restored successfully.")
            } else {
                window.notification = appController.backup.lastError
            }
            notificationTimer.restart()
        }
    }

    FileDialog {
        id: databaseBackupDialog
        title: qsTr("Create database backup")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "db"
        nameFilters: [qsTr("Lumina database backup (*.db)")]
        onAccepted: {
            var path = window.localPath(selectedFile)
            window.notification = appController.backup.backupDatabase(path)
                ? qsTr("Database backup created.") : appController.backup.lastError
            notificationTimer.restart()
        }
    }

    header: ToolBar {
        contentHeight: 64
        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 28
            spacing: 12

            Image {
                source: "qrc:/qt/qml/LuminaReader/assets/lumina-logo.png"
                sourceSize.width: 40
                sourceSize.height: 40
                fillMode: Image.PreserveAspectFit
                smooth: true
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
            }

            ColumnLayout {
                spacing: 0
                Label {
                    text: qsTr("Lumina Reader")
                    color: Theme.textPrimary
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                }
                Label {
                    text: qsTr("Your personal reading space")
                    color: Theme.textSecondary
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                text: qsTr("v%1").arg(appController.applicationVersion)
                color: Theme.textSecondary
                font.pixelSize: 13
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 232
            Layout.fillHeight: true
            color: Theme.sidebar
            border.color: Theme.border
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8

                Label {
                    text: qsTr("Library")
                    color: Theme.textSecondary
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    Layout.leftMargin: 12
                    Layout.topMargin: 8
                }

                Button {
                    Layout.fillWidth: true
                    checkable: true
                    checked: window.currentView === 0
                    onClicked: window.currentView = 0
                    contentItem: RowLayout {
                        spacing: 10
                        Label { text: "▦"; color: parent.parent.checked ? Theme.accent : Theme.textSecondary; font.pixelSize: 18 }
                        Label { text: qsTr("My library"); color: Theme.textPrimary; Layout.fillWidth: true }
                    }
                }

                Button {
                    Layout.fillWidth: true
                    checkable: true
                    checked: window.currentView === 0 && appController.library.viewFilter === "favorites"
                    onClicked: window.setLibraryFilter("favorites")
                    contentItem: RowLayout {
                        spacing: 10
                        Label { text: "★"; color: parent.parent.checked ? Theme.accent : Theme.textSecondary; font.pixelSize: 18 }
                        Label { text: qsTr("Favorites"); color: Theme.textPrimary; Layout.fillWidth: true }
                    }
                }

                Button {
                    Layout.fillWidth: true
                    checkable: true
                    checked: window.currentView === 0 && appController.library.viewFilter === "to_read"
                    onClicked: window.setLibraryFilter("to_read")
                    contentItem: RowLayout {
                        spacing: 10
                        Label { text: "○"; color: parent.parent.checked ? Theme.accent : Theme.textSecondary; font.pixelSize: 18 }
                        Label { text: qsTr("To read"); color: Theme.textPrimary; Layout.fillWidth: true }
                    }
                }

                Button {
                    Layout.fillWidth: true
                    checkable: true
                    checked: window.currentView === 0 && appController.library.viewFilter === "reading"
                    onClicked: window.setLibraryFilter("reading")
                    contentItem: RowLayout {
                        spacing: 10
                        Label { text: "◐"; color: parent.parent.checked ? Theme.accent : Theme.textSecondary; font.pixelSize: 18 }
                        Label { text: qsTr("Reading"); color: Theme.textPrimary; Layout.fillWidth: true }
                    }
                }

                Button {
                    Layout.fillWidth: true
                    checkable: true
                    checked: window.currentView === 0 && appController.library.viewFilter === "read"
                    onClicked: window.setLibraryFilter("read")
                    contentItem: RowLayout {
                        spacing: 10
                        Label { text: "✓"; color: parent.parent.checked ? Theme.accent : Theme.textSecondary; font.pixelSize: 18 }
                        Label { text: qsTr("Finished"); color: Theme.textPrimary; Layout.fillWidth: true }
                    }
                }

                Button {
                    Layout.fillWidth: true
                    checkable: true
                    checked: window.currentView === 1
                    onClicked: window.currentView = 1
                    contentItem: RowLayout {
                        spacing: 10
                        Label { text: "✦"; color: parent.parent.checked ? Theme.accent : Theme.textSecondary; font.pixelSize: 18 }
                        Label { text: qsTr("Knowledge center"); color: Theme.textPrimary; Layout.fillWidth: true }
                    }
                }

                Label {
                    text: qsTr("Workspace")
                    color: Theme.textSecondary
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    Layout.leftMargin: 12
                    Layout.topMargin: 18
                }

                Button {
                    Layout.fillWidth: true
                    checkable: true
                    checked: window.currentView === 3
                    onClicked: window.currentView = 3
                    contentItem: RowLayout {
                        spacing: 10
                        Label { text: "⚙"; color: parent.parent.checked ? Theme.accent : Theme.textSecondary; font.pixelSize: 18 }
                        Label { text: qsTr("Settings"); color: Theme.textPrimary; Layout.fillWidth: true }
                    }
                }

                Item { Layout.fillHeight: true }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 86
                    radius: 12
                    color: Theme.surfaceMuted
                    border.color: Theme.border

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 4
                        Label {
                            text: qsTr("Private by design")
                            color: Theme.textPrimary
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                        Label {
                            text: qsTr("Your library stays on this device.")
                            color: Theme.textSecondary
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }

        StackLayout {
            id: pages
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: window.currentView

            Item {
                id: libraryPage
                Layout.fillWidth: true
                Layout.fillHeight: true

                DropArea {
                    anchors.fill: parent
                    keys: ["text/uri-list"]
                    onDropped: {
                        if (drop.hasUrls) {
                            window.openImportedFiles(drop.urls)
                        }
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 32
                    spacing: 20

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 14
                        ColumnLayout {
                            spacing: 2
                            Label {
                                text: qsTr("Welcome back")
                                color: Theme.textSecondary
                                font.pixelSize: 14
                            }
                            Label {
                                text: qsTr("Your library")
                                color: Theme.textPrimary
                                font.pixelSize: 30
                                font.weight: Font.DemiBold
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: qsTr("Add documents")
                            highlighted: true
                            onClicked: importDialog.open()
                        }
                        Button {
                            text: qsTr("Add folder")
                            onClicked: importFolderDialog.open()
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        TextField {
                            id: librarySearch
                            Layout.fillWidth: true
                            placeholderText: qsTr("Search your library by title, author, or text")
                            onTextChanged: appController.library.query = text
                        }
                        Label {
                            text: qsTr("%1 item(s)").arg(appController.library.count)
                            color: Theme.textSecondary
                            font.pixelSize: 13
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 16
                        color: Theme.surface
                        border.color: Theme.border

                        GridView {
                            id: libraryGrid
                            anchors.fill: parent
                            anchors.margins: 18
                            clip: true
                            cellWidth: 238
                            cellHeight: 246
                            model: appController.library

                            delegate: Item {
                                width: libraryGrid.cellWidth - 12
                                height: libraryGrid.cellHeight - 12
                                property string documentId: model.id
                                property bool documentFavorite: model.isFavorite
                                property int documentRating: model.rating

                                Rectangle {
                                    anchors.fill: parent
                                    radius: 14
                                    color: cardMouse.containsMouse ? Theme.surfaceRaised : Theme.surface
                                    border.color: cardMouse.containsMouse ? Theme.accentSoft : Theme.border

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 14
                                        spacing: 8

                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 118
                                            radius: 10
                                            color: Theme.coverBackground
                                            clip: true

                                            Image {
                                                anchors.centerIn: parent
                                                source: model.coverPath && String(model.coverPath).length > 0
                                                    ? window.fileUrl(model.coverPath)
                                                    : "qrc:/qt/qml/LuminaReader/assets/lumina-logo.png"
                                                sourceSize.width: 70
                                                sourceSize.height: 70
                                                opacity: 0.78
                                            }

                                            Label {
                                                anchors.left: parent.left
                                                anchors.top: parent.top
                                                anchors.margins: 8
                                                text: String(model.format).toUpperCase()
                                                color: Theme.accent
                                                font.pixelSize: 10
                                                font.weight: Font.DemiBold
                                            }
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: model.title
                                            color: Theme.textPrimary
                                            font.pixelSize: 15
                                            font.weight: Font.DemiBold
                                            elide: Text.ElideRight
                                        }
                                        Label {
                                            Layout.fillWidth: true
                                            text: window.statusLabel(model.status)
                                            color: Theme.textSecondary
                                            font.pixelSize: 12
                                        }
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 0
                                            Repeater {
                                                model: 5
                                                delegate: ToolButton {
                                                    text: index < documentRating ? "★" : "☆"
                                                    font.pixelSize: 13
                                                    padding: 0
                                                    display: AbstractButton.TextOnly
                                                    onClicked: appController.library.setRating(documentId, index + 1)
                                                }
                                            }
                                            Item { Layout.fillWidth: true }
                                        }
                                        ProgressBar {
                                            Layout.fillWidth: true
                                            value: model.progress
                                        }
                                    }

                                    MouseArea {
                                        id: cardMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            if (appController.reader.openDocument(documentId)) {
                                                window.currentView = 2
                                            }
                                        }
                                    }

                                     ToolButton {
                                         anchors.top: parent.top
                                         anchors.right: parent.right
                                         anchors.margins: 6
                                         z: 2
                                         text: documentFavorite ? "★" : "☆"
                                         font.pixelSize: 20
                                         display: AbstractButton.TextOnly
                                         onClicked: appController.library.setFavorite(documentId, !documentFavorite)
                                     }

                                     ToolButton {
                                         anchors.top: parent.top
                                         anchors.left: parent.left
                                         anchors.margins: 6
                                         z: 2
                                         text: "✎"
                                         font.pixelSize: 17
                                         display: AbstractButton.TextOnly
                                         onClicked: window.openMetadata(documentId)
                                     }
                                }
                            }

                            Label {
                                anchors.centerIn: parent
                                visible: appController.library.count === 0
                                text: librarySearch.text.length > 0
                                    ? qsTr("No documents match your search.")
                                    : qsTr("Your library is empty. Drop documents here or choose Add documents.")
                                color: Theme.textSecondary
                                font.pixelSize: 15
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                width: Math.min(parent.width - 80, 420)
                            }
                        }
                    }
                }
            }

            Item {
                id: knowledgePage
                Layout.fillWidth: true
                Layout.fillHeight: true
                property var results: []

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 32
                    spacing: 18

                    Label {
                        text: qsTr("Knowledge center")
                        color: Theme.textPrimary
                        font.pixelSize: 30
                        font.weight: Font.DemiBold
                    }
                    Label {
                        text: qsTr("Find the ideas, passages, and notes you saved across your library.")
                        color: Theme.textSecondary
                        font.pixelSize: 15
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.fillWidth: true }
                        Button {
                            text: qsTr("Export knowledge")
                            enabled: knowledgePage.results.length > 0
                            onClicked: knowledgeExportDialog.open()
                        }
                    }
                    TextField {
                        id: knowledgeSearch
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search highlights and notes")
                        onTextChanged: knowledgePage.results = appController.library.knowledgeSearch(text)
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 16
                        color: Theme.surface
                        border.color: Theme.border

                        ListView {
                            anchors.fill: parent
                            anchors.margins: 18
                            clip: true
                            spacing: 10
                            model: knowledgePage.results

                            delegate: Rectangle {
                                width: ListView.view.width
                                implicitHeight: resultColumn.implicitHeight + 26
                                radius: 12
                                color: Theme.surfaceMuted
                                border.color: Theme.border

                                ColumnLayout {
                                    id: resultColumn
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 13
                                    spacing: 4
                                    Label {
                                        text: modelData.documentTitle
                                        color: Theme.textPrimary
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                    }
                                    Label {
                                        text: "“%1”".arg(modelData.text)
                                        color: Theme.textPrimary
                                        font.pixelSize: 14
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                    Label {
                                        visible: modelData.note.length > 0
                                        text: qsTr("Note: %1").arg(modelData.note)
                                        color: Theme.textSecondary
                                        font.pixelSize: 12
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                     Label {
                                         text: qsTr("Page %1 · %2").arg(Number(modelData.page) + 1).arg(modelData.kind === "annotation" ? qsTr("Note") : qsTr("Highlight"))
                                         color: Theme.textSecondary
                                         font.pixelSize: 11
                                     }
                                }

                                 MouseArea {
                                     anchors.fill: parent
                                     onClicked: {
                                         if (appController.reader.openDocument(modelData.documentId)) {
                                             appController.reader.goToPage(Number(modelData.page))
                                             window.currentView = 2
                                         }
                                     }
                                 }
                            }

                            Label {
                                anchors.centerIn: parent
                                visible: knowledgePage.results.length === 0
                                text: knowledgeSearch.text.length === 0
                                    ? qsTr("Your saved knowledge will appear here.")
                                    : qsTr("No saved passages match this search.")
                                color: Theme.textSecondary
                            }
                        }
                    }
                }
            }

            Item {
                id: readerPage
                Layout.fillWidth: true
                Layout.fillHeight: true

                Connections {
                    target: appController.reader
                    function onStateChanged() {
                        if (!appController.reader.open) {
                            documentSearch.text = ""
                            return
                        }
                        Qt.callLater(function() {
                            if (appController.reader.searchStart >= 0 && appController.reader.searchLength > 0) {
                                readerText.select(appController.reader.searchStart,
                                                  appController.reader.searchStart + appController.reader.searchLength)
                            }
                        })
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    ToolBar {
                        Layout.fillWidth: true
                        background: Rectangle { color: Theme.surface; border.color: Theme.border }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 18
                            anchors.rightMargin: 18
                            spacing: 12
                            Button {
                                text: qsTr("Back to library")
                                onClicked: {
                                    appController.reader.closeDocument()
                                    window.currentView = 0
                                }
                            }
                            Label {
                                text: appController.reader.title
                                color: Theme.textPrimary
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Label {
                                visible: appController.reader.open
                                text: appController.reader.engineName
                                color: Theme.textSecondary
                                font.pixelSize: 12
                            }
                            TextField {
                                id: documentSearch
                                Layout.preferredWidth: 180
                                placeholderText: qsTr("Find in document")
                                enabled: appController.reader.open
                                onTextChanged: appController.reader.findInDocument(text)
                                onAccepted: appController.reader.nextSearchMatch()
                            }
                            Label {
                                visible: documentSearch.text.length > 0
                                text: appController.reader.searchMatches > 0
                                    ? qsTr("%1/%2").arg(appController.reader.searchCurrent + 1).arg(appController.reader.searchMatches)
                                    : qsTr("No matches")
                                color: Theme.textSecondary
                                font.pixelSize: 11
                            }
                            Button {
                                text: "‹"
                                enabled: appController.reader.searchMatches > 0
                                onClicked: appController.reader.previousSearchMatch()
                            }
                            Button {
                                text: "›"
                                enabled: appController.reader.searchMatches > 0
                                onClicked: appController.reader.nextSearchMatch()
                            }
                            Button {
                                text: qsTr("Previous")
                                enabled: appController.reader.currentPage > 0
                                onClicked: appController.reader.previousPage()
                            }
                            Button {
                                text: qsTr("Next")
                                enabled: appController.reader.currentPage + 1 < appController.reader.pageCount
                                onClicked: appController.reader.nextPage()
                            }
                             Button {
                                 text: qsTr("Bookmark")
                                 enabled: appController.reader.open
                                 onClicked: {
                                     if (appController.library.addBookmark(appController.reader.documentId,
                                                                           appController.reader.title,
                                                                           "page:" + appController.reader.currentPage,
                                                                           appController.reader.currentPage)) {
                                         window.notification = qsTr("Bookmark saved.")
                                         notificationTimer.restart()
                                     }
                                 }
                             }
                             Button {
                                 text: appController.speech.speaking ? qsTr("Stop") : qsTr("Read aloud")
                                 enabled: appController.reader.open && appController.reader.textDocument
                                     && appController.speech.available
                                 onClicked: {
                                     if (appController.speech.speaking) {
                                         appController.speech.stop()
                                     } else {
                                         appController.speech.speak(readerText.selectedText.length > 0
                                                                    ? readerText.selectedText
                                                                    : appController.reader.textContent)
                                     }
                                 }
                             }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Theme.readerBackground

                        Image {
                            anchors.fill: parent
                            anchors.margins: 28
                            visible: appController.reader.open && appController.reader.imageDocument
                            source: appController.reader.imageSource
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                        }

                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: 28
                            visible: appController.reader.open && appController.reader.textDocument
                            clip: true

                            TextArea {
                                id: readerText
                                width: Math.min(parent.width, 860)
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: appController.reader.textContent
                                readOnly: true
                                selectByMouse: true
                                wrapMode: TextArea.Wrap
                                color: Theme.readerText
                                selectionColor: Theme.highlight
                                selectedTextColor: Theme.textPrimary
                                 font.pixelSize: appController.settings.fontSize
                                font.family: Theme.readerFont
                                 font.letterSpacing: (appController.settings.lineSpacing - 100) / 20
                                padding: 30

                                background: Rectangle {
                                    color: Theme.readerPaper
                                    radius: 10
                                    border.color: Theme.border
                                }
                            }
                        }

                        ColumnLayout {
                            anchors.centerIn: parent
                            visible: !appController.reader.open
                            width: Math.min(parent.width - 64, 520)
                            spacing: 10
                            Label {
                                Layout.fillWidth: true
                                text: appController.reader.errorMessage.length > 0
                                    ? appController.reader.errorMessage
                                    : qsTr("Choose a document from your library to start reading.")
                                color: Theme.textSecondary
                                font.pixelSize: 16
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                            }
                        }

                        RowLayout {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.margins: 18
                            visible: appController.reader.open && readerText.selectedText.length > 0
                            spacing: 8

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 48
                                radius: 12
                                color: Theme.surface
                                border.color: Theme.border
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    Button {
                                        text: qsTr("Highlight")
                                        onClicked: {
                                            appController.library.addHighlight(appController.reader.documentId,
                                                                               readerText.selectedText,
                                                                               "text:" + appController.reader.currentPage,
                                                                               appController.reader.currentPage,
                                                                               Theme.highlight)
                                            readerText.deselect()
                                            notification = qsTr("Passage saved to the Knowledge center.")
                                            notificationTimer.restart()
                                        }
                                    }
                                    Button {
                                        text: qsTr("Copy")
                                        onClicked: readerText.copy()
                                    }
                                     Button {
                                         text: qsTr("Google")
                                         onClicked: {
                                             window.notification = appController.lookup.openGoogle(readerText.selectedText)
                                                 ? qsTr("Opening your browser for this search.")
                                                 : appController.lookup.lastError
                                             notificationTimer.restart()
                                         }
                                     }
                                     Button {
                                         text: qsTr("Dictionary")
                                         onClicked: {
                                             window.notification = appController.lookup.openDictionary(readerText.selectedText)
                                                 ? qsTr("Opening the online dictionary in your browser.")
                                                 : appController.lookup.lastError
                                             notificationTimer.restart()
                                         }
                                     }
                                     Button {
                                         text: qsTr("Translate")
                                         onClicked: {
                                             window.notification = appController.lookup.openTranslation(readerText.selectedText)
                                                 ? qsTr("Opening the online translator in your browser.")
                                                 : appController.lookup.lastError
                                             notificationTimer.restart()
                                         }
                                     }
                                    Item { Layout.fillWidth: true }
                                }
                            }
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 40
                    spacing: 18
                    Label {
                        text: qsTr("Settings")
                        color: Theme.textPrimary
                        font.pixelSize: 30
                        font.weight: Font.DemiBold
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 150
                        radius: 16
                        color: Theme.surface
                        border.color: Theme.border
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 20
                            spacing: 8
                            Label {
                                text: qsTr("Privacy")
                                color: Theme.textPrimary
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                            }
                            Label {
                                text: qsTr("Lumina Reader keeps your library, progress, and saved passages on this device. Online lookups will always be optional and explicit.")
                                color: Theme.textSecondary
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: settingsColumn.implicitHeight + 40
                        radius: 16
                        color: Theme.surface
                        border.color: Theme.border

                        ColumnLayout {
                            id: settingsColumn
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 20
                            spacing: 14

                            Label {
                                text: qsTr("Reading comfort")
                                color: Theme.textPrimary
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: qsTr("Theme"); color: Theme.textPrimary; Layout.fillWidth: true }
                                ComboBox {
                                    Layout.preferredWidth: 190
                                    model: [qsTr("Light"), qsTr("Dark"), qsTr("Sepia"), qsTr("Paper"), qsTr("Gray"), qsTr("High contrast"), qsTr("Low light")]
                                    currentIndex: Math.max(0, window.themeKeys.indexOf(appController.settings.theme))
                                    onActivated: appController.settings.theme = window.themeKeys[index]
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: qsTr("Font size"); color: Theme.textPrimary; Layout.fillWidth: true }
                                Slider {
                                    Layout.preferredWidth: 190
                                    from: 12
                                    to: 32
                                    stepSize: 1
                                    value: appController.settings.fontSize
                                    onMoved: appController.settings.fontSize = Math.round(value)
                                }
                                Label { text: qsTr("%1 px").arg(appController.settings.fontSize); color: Theme.textSecondary; Layout.preferredWidth: 44 }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: qsTr("Line spacing"); color: Theme.textPrimary; Layout.fillWidth: true }
                                Slider {
                                    Layout.preferredWidth: 190
                                    from: 100
                                    to: 180
                                    stepSize: 5
                                    value: appController.settings.lineSpacing
                                    onMoved: appController.settings.lineSpacing = Math.round(value)
                                }
                                Label { text: qsTr("%1%").arg(appController.settings.lineSpacing); color: Theme.textSecondary; Layout.preferredWidth: 44 }
                            }

                            CheckBox {
                                text: qsTr("Use continuous reading when available")
                                checked: appController.settings.continuousReading
                                onToggled: appController.settings.continuousReading = checked
                            }
                            CheckBox {
                                text: qsTr("High contrast controls")
                                checked: appController.settings.highContrast
                                onToggled: appController.settings.highContrast = checked
                            }
                        }
                    }
                    Label {
                        text: qsTr("Your preferences are stored locally with your Lumina Reader library.")
                        color: Theme.textSecondary
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: qsTr("Export library")
                            onClicked: libraryExportDialog.open()
                        }
                        Button {
                            text: qsTr("Create database backup")
                            onClicked: databaseBackupDialog.open()
                        }
                        Button {
                            text: qsTr("Restore library")
                            onClicked: libraryRestoreDialog.open()
                        }
                        Item { Layout.fillWidth: true }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        visible: window.notification.length > 0
        radius: 10
        color: Theme.toast
        implicitWidth: notificationLabel.implicitWidth + 32
        implicitHeight: 44

        Label {
            id: notificationLabel
            anchors.centerIn: parent
            text: window.notification
            color: Theme.toastText
            font.pixelSize: 13
        }
    }
}

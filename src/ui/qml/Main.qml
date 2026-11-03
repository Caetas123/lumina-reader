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
    property bool focusMode: false
    property bool listMode: false
    property real imageZoom: 1.0
    property string zoomDocumentId: ""
    property var highlightColors: ["#f6c945", "#7acb8a", "#77a8e8", "#ed8fb3", "#aa8bd8"]
    property var highlightStyles: ["highlight", "underline", "strike"]
    property var contentTypeKeys: ["book", "manga", "manhwa", "webtoon", "newspaper", "magazine", "article", "academic", "handout", "comic", "document", "image", "text", "other"]
    property var contentTypeLabels: [qsTr("Book"), qsTr("Manga"), qsTr("Manhwa"), qsTr("Webtoon"), qsTr("Newspaper"), qsTr("Magazine"), qsTr("Article"), qsTr("Academic paper"), qsTr("Handout"), qsTr("Comic"), qsTr("Document"), qsTr("Image"), qsTr("Text"), qsTr("Other")]
    property var annotationKindKeys: ["note", "region"]
    property var annotationKindLabels: [qsTr("Note"), qsTr("Image region")]
    property var translationLanguageKeys: ["en", "pt", "es", "fr", "de", "ja", "ko", "zh"]
    property var translationLanguageLabels: [qsTr("English"), qsTr("Portuguese"), qsTr("Spanish"), qsTr("French"), qsTr("German"), qsTr("Japanese"), qsTr("Korean"), qsTr("Chinese")]
    property var ocrLanguageKeys: ["eng", "por", "spa", "jpn", "kor", "chi_sim"]
    property var ocrLanguageLabels: [qsTr("English"), qsTr("Portuguese"), qsTr("Spanish"), qsTr("Japanese"), qsTr("Korean"), qsTr("Chinese")]
    property var libraryBrowseOptions: []
    property var librarySortOptions: [
        { label: qsTr("Recently opened"), value: "recent" },
        { label: qsTr("Date added"), value: "added" },
        { label: qsTr("Title"), value: "title" },
        { label: qsTr("Author"), value: "author" },
        { label: qsTr("Progress"), value: "progress" },
        { label: qsTr("Rating"), value: "rating" }
    ]

    function applyTheme() {
        Theme.mode = appController.settings.highContrast ? "high-contrast" : appController.settings.theme
    }

    Component.onCompleted: {
        applyTheme()
        refreshLibraryBrowseOptions()
    }

    Connections {
        target: appController.settings
        function onSettingsChanged() {
            window.applyTheme()
        }
    }

    Connections {
        target: appController.library
        function onLibraryChanged() {
            window.refreshLibraryBrowseOptions()
            knowledgePage.refreshDocumentOptions()
        }
        function onViewFilterChanged() {
            var filter = appController.library.viewFilter
            for (var index = 0; index < window.libraryBrowseOptions.length; ++index) {
                if (window.libraryBrowseOptions[index].filter === filter) {
                    libraryBrowseFilter.currentIndex = index
                    break
                }
            }
        }
        function onSortModeChanged() {
            var mode = appController.library.sortMode
            for (var index = 0; index < window.librarySortOptions.length; ++index) {
                if (window.librarySortOptions[index].value === mode) {
                    librarySort.currentIndex = index
                    break
                }
            }
        }
    }

    Timer {
        id: librarySearchHistoryTimer
        interval: 650
        repeat: false
        onTriggered: appController.library.recordSearch(librarySearch.text, "library")
    }

    Timer {
        id: knowledgeSearchHistoryTimer
        interval: 650
        repeat: false
        onTriggered: appController.library.recordSearch(knowledgeSearch.text, "knowledge")
    }

    Shortcut {
        sequence: StandardKey.Open
        onActivated: importDialog.open()
    }

    Shortcut {
        sequence: "Ctrl+Shift+O"
        onActivated: importFolderDialog.open()
    }

    Shortcut {
        sequence: StandardKey.Find
        enabled: appController.reader.open
        onActivated: window.focusDocumentSearch()
    }

    Shortcut {
        sequence: "Escape"
        enabled: appController.reader.open
        onActivated: {
            appController.reader.closeDocument()
            window.currentView = 0
            window.focusMode = false
        }
    }

    Shortcut {
        sequence: "Right"
        enabled: window.currentView === 2 && appController.reader.open
        onActivated: appController.reader.nextPage()
    }

    Shortcut {
        sequence: "Left"
        enabled: window.currentView === 2 && appController.reader.open
        onActivated: appController.reader.previousPage()
    }

    Shortcut {
        sequence: "Home"
        enabled: window.currentView === 2 && appController.reader.open
        onActivated: appController.reader.goToPage(0)
    }

    Shortcut {
        sequence: "End"
        enabled: window.currentView === 2 && appController.reader.open
        onActivated: appController.reader.goToPage(appController.reader.pageCount - 1)
    }

    Shortcut {
        sequence: "Ctrl+Shift+B"
        enabled: window.currentView === 2 && appController.reader.open
        onActivated: appController.library.addBookmark(appController.reader.documentId,
                                                        appController.reader.title,
                                                        "page:" + appController.reader.currentPage,
                                                        appController.reader.currentPage)
    }

    Shortcut {
        sequence: "Ctrl+Shift+F"
        enabled: appController.reader.open
        onActivated: window.focusMode = !window.focusMode
    }

    Shortcut {
        sequence: "Ctrl+Shift+T"
        onActivated: window.cycleTheme()
    }

    Shortcut {
        sequence: "F11"
        onActivated: window.toggleFullScreen()
    }

    Shortcut {
        sequence: "Ctrl++"
        enabled: window.currentView === 2 && appController.reader.open && appController.reader.imageDocument
        onActivated: window.changeImageZoom(0.1)
    }

    Shortcut {
        sequence: "Ctrl+="
        enabled: window.currentView === 2 && appController.reader.open && appController.reader.imageDocument
        onActivated: window.changeImageZoom(0.1)
    }

    Shortcut {
        sequence: "Ctrl+-"
        enabled: window.currentView === 2 && appController.reader.open && appController.reader.imageDocument
        onActivated: window.changeImageZoom(-0.1)
    }

    Shortcut {
        sequence: "Ctrl+0"
        enabled: window.currentView === 2 && appController.reader.open && appController.reader.imageDocument
        onActivated: window.resetImageZoom()
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

    function toggleFullScreen() {
        window.visibility = window.visibility === Window.FullScreen
            ? Window.Windowed : Window.FullScreen
    }

    function changeImageZoom(delta) {
        imageZoom = Math.max(0.5, Math.min(3.0, imageZoom + delta))
    }

    function resetImageZoom() {
        imageZoom = 1.0
    }

    function focusDocumentSearch() {
        if (!appController.reader.open) {
            return
        }
        window.currentView = 2
        documentSearch.forceActiveFocus()
        documentSearch.selectAll()
    }

    function openSearchHistory(scope) {
        searchHistoryDialog.scope = scope
        searchHistoryDialog.entries = appController.library.searchHistoryForScope(scope)
        searchHistoryDialog.open()
    }

    function useSearchHistory(query) {
        if (searchHistoryDialog.scope === "library") {
            librarySearch.text = query
            appController.library.query = query
        } else if (searchHistoryDialog.scope === "knowledge") {
            knowledgeSearch.text = query
            knowledgePage.refreshResults()
        } else if (searchHistoryDialog.scope === "document") {
            documentSearch.text = query
            appController.reader.findInDocument(query)
        }
        searchHistoryDialog.close()
    }

    function cycleTheme() {
        var current = window.themeKeys.indexOf(appController.settings.theme)
        var next = (current + 1) % window.themeKeys.length
        appController.settings.theme = window.themeKeys[next]
    }

    function openMetadata(documentId) {
        var record = appController.library.documentDetails(documentId)
        if (!record || !record.id) {
            return
        }
        metadataDialog.documentId = documentId
        metadataDialog.coverPath = record.coverPath || ""
        metadataDialog.filePath = record.filePath || ""
        metadataDialog.fileAvailable = appController.library.documentAvailable(documentId)
        metadataContentType.currentIndex = Math.max(0, window.contentTypeKeys.indexOf(record.contentType || "other"))
        metadataTitle.text = record.title || ""
        metadataAuthor.text = record.author || ""
        metadataLanguage.text = record.language || ""
        metadataGenre.text = record.genre || ""
        metadataDescription.text = record.description || ""
        metadataTags.text = appController.library.tagsForDocument(documentId).join(", ")
        collectionSelector.model = appController.library.collections()
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

    function readingTimeLabel(seconds) {
        var totalMinutes = Math.floor(Math.max(0, Number(seconds) || 0) / 60)
        if (totalMinutes < 60) {
            return qsTr("%1 min read").arg(totalMinutes)
        }
        var hours = Math.floor(totalMinutes / 60)
        var minutes = totalMinutes % 60
        return minutes === 0
            ? qsTr("%1 h read").arg(hours)
            : qsTr("%1 h %2 min read").arg(hours).arg(minutes)
    }

    function refreshLibraryBrowseOptions() {
        var options = [
            { label: qsTr("All items"), filter: "all" },
            { label: qsTr("Recently opened"), filter: "recent" },
            { label: qsTr("Favorites"), filter: "favorites" },
            { label: qsTr("To read"), filter: "to_read" },
            { label: qsTr("Reading"), filter: "reading" },
            { label: qsTr("Finished"), filter: "read" },
            { label: qsTr("Abandoned"), filter: "abandoned" }
        ]
        for (var typeIndex = 0; typeIndex < window.contentTypeKeys.length; ++typeIndex) {
            options.push({
                label: window.contentTypeLabels[typeIndex],
                filter: "type:" + window.contentTypeKeys[typeIndex]
            })
        }
        var collections = appController.library.collections()
        for (var collectionIndex = 0; collectionIndex < collections.length; ++collectionIndex) {
            options.push({
                label: qsTr("Collection: %1").arg(collections[collectionIndex].name),
                filter: "collection:" + collections[collectionIndex].id
            })
        }
        var tags = appController.library.tags()
        for (var tagIndex = 0; tagIndex < tags.length; ++tagIndex) {
            options.push({
                label: qsTr("Tag: %1").arg(tags[tagIndex].name),
                filter: "tag:" + tags[tagIndex].name
            })
        }
        var genres = appController.library.genres()
        for (var genreIndex = 0; genreIndex < genres.length; ++genreIndex) {
            options.push({
                label: qsTr("Genre: %1").arg(genres[genreIndex]),
                filter: "genre:" + genres[genreIndex]
            })
        }
        var languages = appController.library.languages()
        for (var languageIndex = 0; languageIndex < languages.length; ++languageIndex) {
            options.push({
                label: qsTr("Language: %1").arg(languages[languageIndex]),
                filter: "language:" + languages[languageIndex]
            })
        }
        var formats = appController.library.formats()
        for (var formatIndex = 0; formatIndex < formats.length; ++formatIndex) {
            options.push({
                label: qsTr("Format: %1").arg(String(formats[formatIndex]).toUpperCase()),
                filter: "format:" + formats[formatIndex]
            })
        }
        libraryBrowseOptions = options
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

    FileDialog {
        id: relinkDialog
        property string documentId: ""
        title: qsTr("Locate the original document")
        fileMode: FileDialog.OpenFile
        nameFilters: [
            qsTr("Supported documents (%1)").arg(appController.library.supportedExtensions().join(" ")),
            qsTr("All files (*)")
        ]
        onAccepted: {
            var path = window.localPath(selectedFile)
            if (appController.library.relinkDocument(documentId, path)) {
                metadataDialog.filePath = path
                metadataDialog.fileAvailable = true
                window.notification = qsTr("The document location was restored.")
            } else {
                window.notification = qsTr("That file does not match the saved document.")
            }
            notificationTimer.restart()
        }
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

    FolderDialog {
        id: watchFolderDialog
        title: qsTr("Watch a folder for new documents")
        onAccepted: {
            var path = window.localPath(selectedFolder)
            if (appController.library.watchFolder(path, true)) {
                window.notification = qsTr("Lumina Reader will watch this folder for new documents.")
            } else {
                window.notification = qsTr("The selected folder could not be watched.")
            }
            notificationTimer.restart()
        }
    }

    Dialog {
        id: metadataDialog
        property string documentId: ""
        property string coverPath: ""
        property string filePath: ""
        property bool fileAvailable: true
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
            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    text: metadataDialog.fileAvailable
                        ? metadataDialog.filePath
                        : qsTr("Original file not found: %1").arg(metadataDialog.filePath)
                    color: metadataDialog.fileAvailable ? Theme.textSecondary : Theme.warning
                    elide: Text.ElideMiddle
                }
                Button {
                    text: qsTr("Locate file")
                    onClicked: {
                        relinkDialog.documentId = metadataDialog.documentId
                        relinkDialog.open()
                    }
                }
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
            ComboBox {
                id: metadataContentType
                Layout.fillWidth: true
                model: window.contentTypeLabels
            }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: metadataLanguage
                    Layout.fillWidth: true
                    placeholderText: qsTr("Language")
                }
                TextField {
                    id: metadataGenre
                    Layout.fillWidth: true
                    placeholderText: qsTr("Genre")
                }
                TextField {
                    id: metadataTags
                    Layout.fillWidth: true
                    placeholderText: qsTr("Tags, separated by commas")
                }
            }
            RowLayout {
                Layout.fillWidth: true
                ComboBox {
                    id: collectionSelector
                    Layout.fillWidth: true
                    textRole: "name"
                    valueRole: "id"
                    model: []
                    displayText: currentIndex >= 0 ? currentText : qsTr("Choose a collection")
                }
                Button {
                    text: qsTr("Add")
                    enabled: collectionSelector.currentIndex >= 0
                    onClicked: {
                        var added = appController.library.addToCollection(metadataDialog.documentId,
                                                                          collectionSelector.currentValue)
                        window.notification = added
                            ? qsTr("Document added to the collection.")
                            : qsTr("The document could not be added to the collection.")
                        notificationTimer.restart()
                    }
                }
                Button {
                    text: qsTr("Remove")
                    enabled: collectionSelector.currentIndex >= 0
                    onClicked: {
                        var removed = appController.library.removeFromCollection(metadataDialog.documentId,
                                                                                  collectionSelector.currentValue)
                        window.notification = removed
                            ? qsTr("Document removed from the collection.")
                            : qsTr("The document is not in that collection.")
                        notificationTimer.restart()
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    text: metadataDialog.coverPath.length > 0
                        ? metadataDialog.coverPath
                        : qsTr("No cover image selected")
                    color: Theme.textSecondary
                    elide: Text.ElideMiddle
                }
                Button {
                    text: qsTr("Choose cover")
                    onClicked: coverDialog.open()
                }
                Button {
                    text: qsTr("Clear")
                    enabled: metadataDialog.coverPath.length > 0
                    onClicked: {
                        if (appController.library.setCoverPath(metadataDialog.documentId, "")) {
                            metadataDialog.coverPath = ""
                            window.notification = qsTr("The cover image was cleared.")
                            notificationTimer.restart()
                        }
                    }
                }
            }
            TextArea {
                id: metadataDescription
                Layout.fillWidth: true
                Layout.preferredHeight: 110
                placeholderText: qsTr("Personal description")
                wrapMode: TextArea.Wrap
            }
            Button {
                text: qsTr("Remove from library")
                Layout.alignment: Qt.AlignLeft
                onClicked: {
                    removeDocumentDialog.documentId = metadataDialog.documentId
                    removeDocumentDialog.documentTitle = metadataTitle.text
                    removeDocumentDialog.open()
                }
            }
        }

        onAccepted: {
            var saved = appController.library.updateMetadata(documentId,
                                                              metadataTitle.text,
                                                              metadataAuthor.text,
                                                              metadataDescription.text,
                                                              metadataLanguage.text,
                                                               window.contentTypeKeys[metadataContentType.currentIndex],
                                                               metadataGenre.text)
            if (saved) {
                var tags = metadataTags.text.split(",")
                saved = appController.library.setTags(documentId, tags)
            }
            appController.library.refresh()
            window.notification = saved ? qsTr("Document details saved.") : qsTr("The document details could not be saved.")
            notificationTimer.restart()
        }
    }

    Dialog {
        id: removeDocumentDialog
        property string documentId: ""
        property string documentTitle: ""
        modal: true
        title: qsTr("Remove document")
        width: Math.min(window.width - 80, 520)
        standardButtons: Dialog.Ok | Dialog.Cancel

        contentItem: ColumnLayout {
            spacing: 12
            Label {
                Layout.fillWidth: true
                text: qsTr("Remove “%1” from your Lumina Reader library? The original file will stay on your device unless you explicitly choose otherwise.").arg(removeDocumentDialog.documentTitle)
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
            }
            CheckBox {
                id: deleteOriginalCheck
                text: qsTr("Also delete the original file from the device")
            }
        }

        onAccepted: {
            var removed = appController.library.removeDocument(documentId, deleteOriginalCheck.checked)
            metadataDialog.close()
            window.notification = removed
                ? qsTr("The document was removed from your library.")
                : qsTr("The document could not be removed.")
            notificationTimer.restart()
            deleteOriginalCheck.checked = false
        }
        onRejected: deleteOriginalCheck.checked = false
    }

    FileDialog {
        id: coverDialog
        title: qsTr("Choose a cover image")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.webp *.avif *.gif)"), qsTr("All files (*)")]
        onAccepted: {
            var path = window.localPath(selectedFile)
            if (appController.library.setCoverPath(metadataDialog.documentId, path)) {
                metadataDialog.coverPath = path
                window.notification = qsTr("The cover image was saved.")
            } else {
                window.notification = qsTr("The selected cover image could not be used.")
            }
            notificationTimer.restart()
        }
    }

    Dialog {
        id: createCollectionDialog
        modal: true
        title: qsTr("Create collection")
        width: Math.min(window.width - 80, 520)
        standardButtons: Dialog.Ok | Dialog.Cancel

        contentItem: ColumnLayout {
            spacing: 12
            TextField {
                id: collectionName
                Layout.fillWidth: true
                placeholderText: qsTr("Collection name")
            }
            TextArea {
                id: collectionDescription
                Layout.fillWidth: true
                Layout.preferredHeight: 90
                placeholderText: qsTr("Description (optional)")
                wrapMode: TextArea.Wrap
            }
        }

        onAccepted: {
            var created = appController.library.createCollection(collectionName.text,
                                                                  collectionDescription.text)
            window.notification = created
                ? qsTr("Collection created.")
                : qsTr("A collection with that name could not be created.")
            notificationTimer.restart()
            collectionName.clear()
            collectionDescription.clear()
        }
        onRejected: {
            collectionName.clear()
            collectionDescription.clear()
        }
    }

    Dialog {
        id: regionAnnotationDialog
        property real regionLeft: 0
        property real regionTop: 0
        property real regionWidth: 0
        property real regionHeight: 0
        modal: true
        title: qsTr("Annotate selected region")
        width: Math.min(window.width - 80, 560)
        standardButtons: Dialog.Ok | Dialog.Cancel

        contentItem: ColumnLayout {
            spacing: 12

            Label {
                text: qsTr("This page has no selectable text. Add a note describing the selected region.")
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            TextArea {
                id: regionAnnotationText
                Layout.fillWidth: true
                Layout.preferredHeight: 130
                placeholderText: qsTr("Write a note for this region")
                wrapMode: TextArea.Wrap
            }
        }

        onOpened: regionAnnotationText.forceActiveFocus()
        onAccepted: {
            var note = regionAnnotationText.text.trim()
            var locator = "region:%1,%2,%3,%4".arg(regionLeft.toFixed(6))
                    .arg(regionTop.toFixed(6)).arg(regionWidth.toFixed(6)).arg(regionHeight.toFixed(6))
            var saved = note.length > 0 && appController.library.addAnnotation(
                appController.reader.documentId,
                note,
                "region",
                locator,
                appController.reader.currentPage,
                regionLeft,
                regionTop,
                regionWidth,
                regionHeight)
            window.notification = saved
                ? qsTr("Region annotation saved to the Knowledge center.")
                : qsTr("The region annotation could not be saved.")
            notificationTimer.restart()
            regionAnnotationText.clear()
            imageRegionLayer.clearSelection()
        }
        onRejected: {
            regionAnnotationText.clear()
            imageRegionLayer.clearSelection()
        }
    }

    Dialog {
        id: textHighlightDialog
        property string editingId: ""
        property string selectedText: ""
        property int selectedPage: 0
        property string selectedLocator: ""
        modal: true
        title: editingId.length > 0 ? qsTr("Edit saved passage") : qsTr("Save passage with a comment")
        width: Math.min(window.width - 80, 560)
        standardButtons: Dialog.Ok | Dialog.Cancel

        contentItem: ColumnLayout {
            spacing: 12
            Label {
                Layout.fillWidth: true
                text: qsTr("“%1”").arg(textHighlightDialog.selectedText)
                color: Theme.textPrimary
                wrapMode: Text.WordWrap
                maximumLineCount: 4
                elide: Text.ElideRight
            }
            RowLayout {
                Layout.fillWidth: true
                ComboBox {
                    id: textHighlightColor
                    Layout.fillWidth: true
                    model: [qsTr("Yellow"), qsTr("Green"), qsTr("Blue"), qsTr("Pink"), qsTr("Purple")]
                }
                ComboBox {
                    id: textHighlightStyle
                    Layout.fillWidth: true
                    model: [qsTr("Highlight"), qsTr("Underline"), qsTr("Strikethrough")]
                }
            }
            TextArea {
                id: textHighlightNote
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                placeholderText: qsTr("Comment about this passage")
                wrapMode: TextArea.Wrap
            }
        }

        onOpened: textHighlightNote.forceActiveFocus()
        onAccepted: {
            var saved = false
            if (textHighlightDialog.editingId.length > 0) {
                saved = textHighlightNote.text.trim().length > 0
                    && appController.library.updateHighlight(
                        textHighlightDialog.editingId,
                        window.highlightColors[textHighlightColor.currentIndex],
                        textHighlightNote.text.trim(),
                        window.highlightStyles[textHighlightStyle.currentIndex])
            } else {
                saved = textHighlightDialog.selectedText.trim().length > 0
                    && textHighlightNote.text.trim().length > 0
                    && appController.library.addHighlight(
                        appController.reader.documentId,
                        textHighlightDialog.selectedText,
                        textHighlightDialog.selectedLocator,
                        textHighlightDialog.selectedPage,
                        window.highlightColors[textHighlightColor.currentIndex],
                        textHighlightNote.text.trim(),
                        window.highlightStyles[textHighlightStyle.currentIndex])
            }
            window.notification = saved
                ? (textHighlightDialog.editingId.length > 0
                    ? qsTr("Passage updated.")
                    : qsTr("Comment saved to the Knowledge center."))
                : (textHighlightDialog.editingId.length > 0
                    ? qsTr("The passage could not be updated.")
                    : qsTr("The passage comment could not be saved."))
            notificationTimer.restart()
            textHighlightDialog.editingId = ""
            textHighlightNote.clear()
            readerText.deselect()
        }
        onRejected: {
            textHighlightDialog.editingId = ""
            textHighlightNote.clear()
            readerText.deselect()
        }
    }

    Dialog {
        id: annotationEditDialog
        property string editingId: ""
        modal: true
        title: qsTr("Edit saved note")
        width: Math.min(window.width - 80, 560)
        standardButtons: Dialog.Ok | Dialog.Cancel

        contentItem: ColumnLayout {
            spacing: 12

            Label {
                text: qsTr("Update the note without changing the original document or its saved position.")
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            ComboBox {
                id: annotationEditKind
                Layout.fillWidth: true
                model: window.annotationKindLabels
            }
            TextArea {
                id: annotationEditText
                Layout.fillWidth: true
                Layout.preferredHeight: 140
                placeholderText: qsTr("Write your note")
                wrapMode: TextArea.Wrap
            }
        }

        onOpened: annotationEditText.forceActiveFocus()
        onAccepted: {
            var text = annotationEditText.text.trim()
            var saved = editingId.length > 0 && text.length > 0
                && appController.library.updateAnnotation(
                    editingId,
                    text,
                    window.annotationKindKeys[annotationEditKind.currentIndex])
            window.notification = saved
                ? qsTr("Note updated.")
                : qsTr("The note could not be updated.")
            notificationTimer.restart()
            knowledgePage.refreshResults()
            editingId = ""
            annotationEditText.clear()
        }
        onRejected: {
            editingId = ""
            annotationEditText.clear()
        }
    }

    Dialog {
        id: searchHistoryDialog
        property string scope: "library"
        property var entries: []
        modal: true
        title: qsTr("Search history")
        width: Math.min(window.width - 80, 620)
        standardButtons: Dialog.Close

        function scopeLabel() {
            if (scope === "document") {
                return qsTr("This document")
            }
            if (scope === "knowledge") {
                return qsTr("Knowledge center")
            }
            return qsTr("Your library")
        }

        onOpened: entries = appController.library.searchHistoryForScope(scope)
        onClosed: entries = []

        contentItem: ColumnLayout {
            spacing: 10

            Label {
                Layout.fillWidth: true
                text: qsTr("Recent searches in %1").arg(searchHistoryDialog.scopeLabel())
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.fillWidth: true
                Button {
                    text: qsTr("Clear history")
                    enabled: searchHistoryDialog.entries.length > 0
                    onClicked: {
                        if (appController.library.clearSearchHistory(searchHistoryDialog.scope)) {
                            searchHistoryDialog.entries = []
                            window.notification = qsTr("Search history cleared.")
                            notificationTimer.restart()
                        }
                    }
                }
                Item { Layout.fillWidth: true }
            }
            Label {
                visible: searchHistoryDialog.entries.length === 0
                Layout.fillWidth: true
                text: qsTr("No saved searches yet.")
                color: Theme.textSecondary
            }
            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(360, Math.max(64, searchHistoryDialog.entries.length * 52))
                visible: searchHistoryDialog.entries.length > 0
                clip: true
                spacing: 8
                model: searchHistoryDialog.entries

                delegate: Rectangle {
                    required property var modelData
                    width: ListView.view.width
                    height: 44
                    radius: 8
                    color: Theme.surfaceMuted
                    border.color: Theme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8
                        Label {
                            Layout.fillWidth: true
                            text: modelData.query
                            color: Theme.textPrimary
                            elide: Text.ElideRight
                        }
                        Label {
                            text: modelData.createdAt
                            color: Theme.textSecondary
                            font.pixelSize: 11
                        }
                        Button {
                            text: qsTr("Use")
                            onClicked: window.useSearchHistory(modelData.query)
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: lookupDialog
        property string mode: "dictionary"
        property string queryText: ""
        property bool showHistory: false
        property bool historyFavoritesOnly: false
        modal: true
        title: mode === "dictionary" ? qsTr("Dictionary") : qsTr("Translation")
        width: Math.min(window.width - 80, 680)
        standardButtons: Dialog.Close

        onOpened: {
            showHistory = false
            historyFavoritesOnly = false
        }

        function historyEntries() {
            var entries = mode === "dictionary"
                ? appController.lookup.dictionaryHistory
                : appController.lookup.translationHistory
            if (!historyFavoritesOnly) {
                return entries
            }
            var favorites = []
            for (var index = 0; index < entries.length; ++index) {
                if (entries[index].favorite) {
                    favorites.push(entries[index])
                }
            }
            return favorites
        }

        function openHistoryEntry(entry) {
            showHistory = false
            if (mode === "dictionary") {
                appController.lookup.lookupDictionary(entry.term, entry.language)
            } else {
                appController.lookup.translateText(entry.sourceText,
                                                   entry.sourceLanguage,
                                                   entry.targetLanguage)
            }
        }

        contentItem: ColumnLayout {
            spacing: 12

            Label {
                visible: appController.lookup.busy
                text: lookupDialog.mode === "dictionary"
                    ? qsTr("Looking up the selected word…")
                    : qsTr("Translating the selected text…")
                color: Theme.textSecondary
                Layout.fillWidth: true
            }
            RowLayout {
                Layout.fillWidth: true
                Button {
                    text: lookupDialog.showHistory ? qsTr("Back to result") : qsTr("History")
                    onClicked: lookupDialog.showHistory = !lookupDialog.showHistory
                }
                Button {
                    visible: lookupDialog.showHistory
                    checkable: true
                    checked: lookupDialog.historyFavoritesOnly
                    text: qsTr("Favorites")
                    onClicked: lookupDialog.historyFavoritesOnly = checked
                }
                Button {
                    visible: lookupDialog.showHistory
                    text: qsTr("Clear history")
                    onClicked: {
                        if (lookupDialog.mode === "dictionary") {
                            appController.lookup.clearDictionaryHistory()
                        } else {
                            appController.lookup.clearTranslationHistory()
                        }
                    }
                }
                Item { Layout.fillWidth: true }
            }
            Label {
                visible: lookupDialog.showHistory && lookupDialog.historyEntries().length === 0
                text: qsTr("No saved lookups yet.")
                color: Theme.textSecondary
                Layout.fillWidth: true
            }
            ListView {
                id: lookupHistoryList
                visible: lookupDialog.showHistory
                Layout.fillWidth: true
                Layout.preferredHeight: lookupDialog.showHistory ? 360 : 0
                Layout.minimumHeight: 0
                clip: true
                model: lookupDialog.historyEntries()
                delegate: Rectangle {
                    required property var modelData
                    property var entry: modelData
                    width: lookupHistoryList.width
                    height: 72
                    radius: 10
                    color: Theme.surfaceMuted
                    border.color: Theme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label {
                                Layout.fillWidth: true
                                text: lookupDialog.mode === "dictionary"
                                    ? entry.term
                                    : entry.sourceText
                                color: Theme.textPrimary
                                elide: Text.ElideRight
                                font.weight: Font.DemiBold
                            }
                            Label {
                                Layout.fillWidth: true
                                text: lookupDialog.mode === "dictionary"
                                    ? qsTr("%1 · %2").arg(entry.language || qsTr("Auto")).arg(entry.provider)
                                    : qsTr("%1 → %2 · %3").arg(entry.sourceLanguage || qsTr("Auto"))
                                        .arg(entry.targetLanguage).arg(entry.provider)
                                color: Theme.textSecondary
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }
                        Button {
                            text: entry.favorite ? "★" : "☆"
                            Accessible.name: entry.favorite ? qsTr("Remove favorite") : qsTr("Save favorite")
                            onClicked: {
                                if (lookupDialog.mode === "dictionary") {
                                    appController.lookup.setDictionaryFavorite(entry.id, !entry.favorite)
                                } else {
                                    appController.lookup.setTranslationFavorite(entry.id, !entry.favorite)
                                }
                            }
                        }
                        Button {
                            text: qsTr("Open")
                            onClicked: lookupDialog.openHistoryEntry(entry)
                        }
                    }
                }
            }
            Label {
                visible: !lookupDialog.showHistory
                    && !appController.lookup.busy && appController.lookup.lastError.length > 0
                text: appController.lookup.lastError
                color: Theme.warning
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                visible: !lookupDialog.showHistory
                    && !appController.lookup.busy && appController.lookup.resultTitle.length > 0
                text: appController.lookup.resultTitle
                color: Theme.textPrimary
                font.pixelSize: 20
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            Label {
                visible: !lookupDialog.showHistory && lookupDialog.mode === "dictionary"
                    && !appController.lookup.busy
                    && appController.lookup.resultPronunciation.length > 0
                text: appController.lookup.resultPronunciation
                color: Theme.textSecondary
                Layout.fillWidth: true
            }
            ScrollView {
                visible: !lookupDialog.showHistory
                    && !appController.lookup.busy && appController.lookup.resultBody.length > 0
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(420, Math.max(100, lookupResultText.implicitHeight + 20))
                TextArea {
                    id: lookupResultText
                    text: appController.lookup.resultBody
                    readOnly: true
                    wrapMode: TextArea.Wrap
                    color: Theme.textPrimary
                    background: Rectangle {
                        color: Theme.surfaceMuted
                        radius: 10
                        border.color: Theme.border
                    }
                }
            }
            Label {
                visible: !lookupDialog.showHistory
                    && !appController.lookup.busy && appController.lookup.resultSource.length > 0
                text: qsTr("Source: %1. The selected text was sent only because you requested this lookup.")
                    .arg(appController.lookup.resultSource)
                color: Theme.textSecondary
                font.pixelSize: 11
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Button {
                visible: !lookupDialog.showHistory && !appController.lookup.busy
                    && (lookupDialog.mode === "dictionary"
                        ? appController.lookup.resultDictionaryHistoryAvailable
                        : appController.lookup.resultTranslationHistoryAvailable)
                text: lookupDialog.mode === "dictionary"
                    ? (appController.lookup.resultDictionaryFavorite
                        ? qsTr("Remove from favorites") : qsTr("Save to favorites"))
                    : (appController.lookup.resultTranslationFavorite
                        ? qsTr("Remove from favorites") : qsTr("Save to favorites"))
                onClicked: {
                    if (lookupDialog.mode === "dictionary") {
                        appController.lookup.setCurrentDictionaryFavorite(
                            !appController.lookup.resultDictionaryFavorite)
                    } else {
                        appController.lookup.setCurrentTranslationFavorite(
                            !appController.lookup.resultTranslationFavorite)
                    }
                }
            }
            Button {
                visible: !lookupDialog.showHistory
                    && !appController.lookup.busy && appController.lookup.resultBody.length > 0
                text: qsTr("Copy result")
                onClicked: {
                    lookupResultText.selectAll()
                    lookupResultText.copy()
                    lookupResultText.deselect()
                    window.notification = qsTr("Result copied.")
                    notificationTimer.restart()
                }
            }
        }
    }

    Drawer {
        id: pageDrawer
        width: 260
        height: window.height
        edge: Qt.LeftEdge
        modal: true

        contentItem: ColumnLayout {
            spacing: 10
            Label {
                text: appController.reader.imageDocument ? qsTr("Pages") : qsTr("Contents")
                color: Theme.textPrimary
                font.pixelSize: 18
                font.weight: Font.DemiBold
                Layout.leftMargin: 12
                Layout.rightMargin: 12
            }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 8
                model: appController.reader.pageCount

                delegate: Rectangle {
                    width: ListView.view.width - 20
                    height: appController.reader.imageDocument ? 144 : 58
                    anchors.horizontalCenter: parent.horizontalCenter
                    radius: 8
                    color: Number(index) === appController.reader.currentPage
                        ? Theme.accentSoft
                        : Theme.surfaceMuted
                    border.color: Number(index) === appController.reader.currentPage
                        ? Theme.accent
                        : Theme.border

                    Image {
                        visible: appController.reader.imageDocument
                        anchors.top: parent.top
                        anchors.topMargin: 6
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width - 12
                        height: 108
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        source: appController.reader.thumbnailRevision >= 0
                            ? appController.reader.thumbnailSource(index)
                            : ""
                    }
                    Label {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 8
                        text: {
                            var name = appController.reader.pageName(Number(index))
                            return name.length > 0
                                ? qsTr("%1 · Page %2").arg(name).arg(Number(index) + 1)
                                : qsTr("Page %1").arg(Number(index) + 1)
                        }
                        color: Theme.textSecondary
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            appController.reader.goToPage(Number(index))
                            pageDrawer.close()
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: knowledgeDeleteDialog
        property string pendingId: ""
        property string pendingKind: ""
        modal: true
        title: qsTr("Remove saved item?")
        width: Math.min(window.width - 80, 480)
        standardButtons: Dialog.Yes | Dialog.No

        contentItem: Label {
            text: qsTr("This removes the saved item from the Knowledge center. The original document will not be changed.")
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }

        onAccepted: {
            var removed = pendingKind === "highlight"
                ? appController.library.removeHighlight(pendingId)
                : appController.library.removeAnnotation(pendingId)
            window.notification = removed
                ? qsTr("Saved item removed.")
                : qsTr("The saved item could not be removed.")
            notificationTimer.restart()
            knowledgePage.refreshResults()
            pendingId = ""
            pendingKind = ""
        }
        onRejected: {
            pendingId = ""
            pendingKind = ""
        }
    }

    Dialog {
        id: bookmarksDialog
        property var entries: []
        modal: true
        title: qsTr("Bookmarks")
        width: Math.min(window.width - 80, 620)
        standardButtons: Dialog.Close

        onOpened: entries = appController.library.bookmarksForDocument(appController.reader.documentId)
        onClosed: entries = []

        contentItem: ColumnLayout {
            spacing: 10

            Label {
                visible: bookmarksDialog.entries.length === 0
                text: qsTr("No bookmarks have been saved for this document yet.")
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(360, Math.max(64, bookmarksDialog.entries.length * 54))
                visible: bookmarksDialog.entries.length > 0
                clip: true
                spacing: 8
                model: bookmarksDialog.entries

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 46
                    radius: 8
                    color: Theme.surfaceMuted
                    border.color: Theme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8
                        Label {
                            text: String(modelData.label || qsTr("Page %1").arg(Number(modelData.page) + 1))
                            color: Theme.textPrimary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: qsTr("Page %1").arg(Number(modelData.page) + 1)
                            color: Theme.textSecondary
                        }
                        Button {
                            text: qsTr("Go")
                            onClicked: {
                                appController.reader.goToPage(Number(modelData.page))
                                bookmarksDialog.close()
                            }
                        }
                        Button {
                            text: qsTr("Delete")
                            onClicked: {
                                if (appController.library.removeBookmark(modelData.id)) {
                                    bookmarksDialog.entries = appController.library.bookmarksForDocument(appController.reader.documentId)
                                    window.notification = qsTr("Bookmark removed.")
                                } else {
                                    window.notification = qsTr("The bookmark could not be removed.")
                                }
                                notificationTimer.restart()
                            }
                        }
                    }
                }
            }
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
        id: settingsExportDialog
        title: qsTr("Export reading preferences")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("Lumina settings JSON (*.json)")]
        onAccepted: {
            var path = window.localPath(selectedFile)
            window.notification = appController.backup.exportSettings(path)
                ? qsTr("Reading preferences exported successfully.") : appController.backup.lastError
            notificationTimer.restart()
        }
    }

    FileDialog {
        id: settingsRestoreDialog
        title: qsTr("Restore reading preferences")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Lumina settings JSON (*.json)"), qsTr("All files (*)")]
        onAccepted: {
            var path = window.localPath(selectedFile)
            if (appController.backup.restoreSettings(path)) {
                appController.settings.reload()
                appController.speech.reload()
                window.notification = qsTr("Reading preferences restored successfully.")
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
            id: sidebar
            Layout.preferredWidth: 232
            Layout.fillHeight: true
            visible: !window.focusMode
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
                    checked: window.currentView === 0 && appController.library.viewFilter === "abandoned"
                    onClicked: window.setLibraryFilter("abandoned")
                    contentItem: RowLayout {
                        spacing: 10
                        Label { text: "×"; color: parent.parent.checked ? Theme.accent : Theme.textSecondary; font.pixelSize: 18 }
                        Label { text: qsTr("Abandoned"); color: Theme.textPrimary; Layout.fillWidth: true }
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
                        Button {
                            text: qsTr("Watch folder")
                            onClicked: watchFolderDialog.open()
                        }
                        Button {
                            text: window.listMode ? qsTr("Grid view") : qsTr("List view")
                            onClicked: window.listMode = !window.listMode
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        ComboBox {
                            id: libraryBrowseFilter
                            Layout.preferredWidth: 180
                            model: window.libraryBrowseOptions
                            textRole: "label"
                            valueRole: "filter"
                            onActivated: window.setLibraryFilter(currentValue)
                        }
                        ComboBox {
                            id: librarySort
                            Layout.preferredWidth: 150
                            model: window.librarySortOptions
                            textRole: "label"
                            valueRole: "value"
                            currentIndex: Math.max(0, window.librarySortOptions.findIndex(function(option) {
                                return option.value === appController.library.sortMode
                            }))
                            onActivated: appController.library.sortMode = currentValue
                        }
                        TextField {
                            id: librarySearch
                            Layout.fillWidth: true
                            placeholderText: qsTr("Search your library by title, author, or text")
                            onTextChanged: {
                                appController.library.query = text
                                librarySearchHistoryTimer.restart()
                            }
                        }
                        Button {
                            text: qsTr("History")
                            onClicked: window.openSearchHistory("library")
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
                            visible: !window.listMode
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
                                        Label {
                                            visible: Number(model.readingSeconds) > 0
                                            Layout.fillWidth: true
                                            text: window.readingTimeLabel(model.readingSeconds)
                                            color: Theme.textSecondary
                                            font.pixelSize: 11
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

                        ListView {
                            id: libraryList
                            anchors.fill: parent
                            anchors.margins: 18
                            clip: true
                            visible: window.listMode
                            spacing: 8
                            model: appController.library

                            delegate: Rectangle {
                                width: libraryList.width
                                height: 94
                                radius: 12
                                color: listMouse.containsMouse ? Theme.surfaceRaised : Theme.surfaceMuted
                                border.color: listMouse.containsMouse ? Theme.accentSoft : Theme.border

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 12

                                    Image {
                                        Layout.preferredWidth: 54
                                        Layout.preferredHeight: 70
                                        source: model.coverPath && String(model.coverPath).length > 0
                                            ? window.fileUrl(model.coverPath)
                                            : "qrc:/qt/qml/LuminaReader/assets/lumina-logo.png"
                                        sourceSize.width: 70
                                        sourceSize.height: 70
                                        fillMode: Image.PreserveAspectFit
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 3
                                        Label {
                                            text: model.title
                                            color: Theme.textPrimary
                                            font.pixelSize: 15
                                            font.weight: Font.DemiBold
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        Label {
                                            text: (model.author && String(model.author).length > 0
                                                   ? model.author + " · " : "")
                                                + String(model.format).toUpperCase()
                                            color: Theme.textSecondary
                                            font.pixelSize: 12
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        ProgressBar {
                                            Layout.fillWidth: true
                                            value: model.progress
                                        }
                                    }

                                    Label {
                                        text: window.statusLabel(model.status)
                                        color: Theme.textSecondary
                                        font.pixelSize: 12
                                        Layout.preferredWidth: 72
                                    }
                                        Label {
                                            visible: Number(model.readingSeconds) > 0
                                            text: window.readingTimeLabel(model.readingSeconds)
                                            color: Theme.textSecondary
                                            font.pixelSize: 11
                                            Layout.preferredWidth: 112
                                        }

                                    ToolButton {
                                        z: 2
                                        text: model.isFavorite ? "★" : "☆"
                                        font.pixelSize: 20
                                        display: AbstractButton.TextOnly
                                        onClicked: appController.library.setFavorite(model.id, !model.isFavorite)
                                    }
                                    ToolButton {
                                        z: 2
                                        text: "✎"
                                        font.pixelSize: 17
                                        display: AbstractButton.TextOnly
                                        onClicked: window.openMetadata(model.id)
                                    }
                                }

                                MouseArea {
                                    id: listMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        if (appController.reader.openDocument(model.id)) {
                                            window.currentView = 2
                                        }
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
                property var documentOptions: []
                property var formatFilters: ["", "epub", "pdf", "mobi", "azw", "azw3", "fb2", "txt", "rtf", "cbz", "cbr", "cb7", "cbt", "rar", "7z", "zip"]
                property var colorFilters: [""].concat(window.highlightColors)
                property var orderFilters: ["relevance", "newest", "oldest"]

                Component.onCompleted: refreshDocumentOptions()

                function refreshDocumentOptions() {
                    var options = [{ label: qsTr("All documents"), id: "" }]
                    var documents = appController.library.knowledgeDocumentOptions()
                    for (var index = 0; index < documents.length; ++index) {
                        var document = documents[index]
                        var label = document.title
                        if (document.author && document.author.length > 0) {
                            label += " · " + document.author
                        }
                        options.push({ label: label, id: document.id })
                    }
                    documentOptions = options
                }

                function refreshResults() {
                    var documentId = knowledgeDocumentFilter.currentIndex >= 0
                        && knowledgeDocumentFilter.currentIndex < documentOptions.length
                        ? documentOptions[knowledgeDocumentFilter.currentIndex].id : ""
                    results = appController.library.knowledgeSearch(
                        knowledgeSearch.text,
                        formatFilters[knowledgeFormatFilter.currentIndex],
                        colorFilters[knowledgeColorFilter.currentIndex],
                        documentId,
                        knowledgeAuthor.text,
                        knowledgeCreatedAfter.text,
                        knowledgeCreatedBefore.text,
                        orderFilters[knowledgeOrder.currentIndex])
                }

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
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            id: knowledgeSearch
                            Layout.fillWidth: true
                            placeholderText: qsTr("Search highlights and notes")
                            onTextChanged: {
                                knowledgePage.refreshResults()
                                knowledgeSearchHistoryTimer.restart()
                            }
                        }
                        Button {
                            text: qsTr("History")
                            onClicked: window.openSearchHistory("knowledge")
                        }
                        ComboBox {
                            id: knowledgeFormatFilter
                            Layout.preferredWidth: 140
                            model: [qsTr("All formats"), qsTr("EPUB"), qsTr("PDF"), qsTr("MOBI"),
                                    qsTr("AZW"), qsTr("AZW3"), qsTr("FB2"), qsTr("TXT"),
                                    qsTr("RTF"), qsTr("CBZ"), qsTr("CBR"), qsTr("CB7"), qsTr("CBT"),
                                    qsTr("RAR"), qsTr("7Z"), qsTr("ZIP")]
                            onActivated: knowledgePage.refreshResults()
                        }
                        ComboBox {
                            id: knowledgeColorFilter
                            Layout.preferredWidth: 140
                            model: [qsTr("All colors"), qsTr("Yellow"), qsTr("Green"), qsTr("Blue"),
                                    qsTr("Pink"), qsTr("Purple")]
                            onActivated: knowledgePage.refreshResults()
                        }
                        ComboBox {
                            id: knowledgeDocumentFilter
                            Layout.preferredWidth: 190
                            model: knowledgePage.documentOptions
                            textRole: "label"
                            valueRole: "id"
                            onActivated: knowledgePage.refreshResults()
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            id: knowledgeAuthor
                            Layout.fillWidth: true
                            placeholderText: qsTr("Filter by author")
                            onTextChanged: knowledgePage.refreshResults()
                        }
                        TextField {
                            id: knowledgeCreatedAfter
                            Layout.preferredWidth: 150
                            placeholderText: qsTr("From YYYY-MM-DD")
                            onTextChanged: knowledgePage.refreshResults()
                        }
                        TextField {
                            id: knowledgeCreatedBefore
                            Layout.preferredWidth: 150
                            placeholderText: qsTr("To YYYY-MM-DD")
                            onTextChanged: knowledgePage.refreshResults()
                        }
                        ComboBox {
                            id: knowledgeOrder
                            Layout.preferredWidth: 140
                            model: [qsTr("Relevance"), qsTr("Newest"), qsTr("Oldest")]
                            onActivated: knowledgePage.refreshResults()
                        }
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
                                    z: 1
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 13
                                    spacing: 4
                                    Label {
                                        text: modelData.documentTitle + " · " + String(modelData.format).toUpperCase()
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
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8
                                        Label {
                                            text: modelData.author.length > 0
                                                ? qsTr("Author: %1 · %2").arg(modelData.author).arg(modelData.createdAt)
                                                : modelData.createdAt
                                            color: Theme.textSecondary
                                            font.pixelSize: 11
                                            Layout.fillWidth: true
                                        }
                                        Button {
                                            visible: modelData.kind === "highlight"
                                            text: qsTr("Edit")
                                            onClicked: {
                                                textHighlightDialog.editingId = modelData.id
                                                textHighlightDialog.selectedText = modelData.text
                                                textHighlightDialog.selectedPage = Number(modelData.page)
                                                textHighlightDialog.selectedLocator = modelData.locator
                                                textHighlightColor.currentIndex = Math.max(0, window.highlightColors.indexOf(modelData.color))
                                                textHighlightStyle.currentIndex = Math.max(0, window.highlightStyles.indexOf(modelData.style))
                                                textHighlightNote.text = modelData.note || ""
                                                textHighlightDialog.open()
                                            }
                                        }
                                        Button {
                                             visible: modelData.kind === "annotation"
                                             text: qsTr("Edit")
                                             onClicked: {
                                                 annotationEditDialog.editingId = modelData.id
                                                 annotationEditText.text = modelData.text || ""
                                                 annotationEditKind.currentIndex = Math.max(
                                                     0,
                                                     window.annotationKindKeys.indexOf(modelData.annotationKind || "note"))
                                                 annotationEditDialog.open()
                                             }
                                         }
                                         Button {
                                            text: qsTr("Delete")
                                            onClicked: {
                                                knowledgeDeleteDialog.pendingId = modelData.id
                                                knowledgeDeleteDialog.pendingKind = modelData.kind
                                                knowledgeDeleteDialog.open()
                                            }
                                        }
                                    }
                                }

                                 MouseArea {
                                     anchors.fill: parent
                                     z: 0
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
                        if (appController.reader.documentId !== window.zoomDocumentId) {
                            window.zoomDocumentId = appController.reader.documentId
                            window.resetImageZoom()
                        }
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
                                onAccepted: {
                                    appController.library.recordSearch(text, "document")
                                    appController.reader.nextSearchMatch()
                                }
                            }
                            Button {
                                visible: appController.reader.open
                                text: qsTr("History")
                                onClicked: window.openSearchHistory("document")
                            }
                            Label {
                                visible: documentSearch.text.length > 0
                                text: appController.reader.searchMatches > 0
                                    ? qsTr("%1/%2").arg(appController.reader.searchCurrent + 1).arg(appController.reader.searchMatches)
                                    : qsTr("No matches")
                                color: Theme.textSecondary
                                font.pixelSize: 11
                            }
                            RowLayout {
                                visible: appController.reader.open
                                spacing: 4
                                Label {
                                    text: qsTr("Page")
                                    color: Theme.textSecondary
                                    font.pixelSize: 11
                                }
                                SpinBox {
                                    id: readerPageNumber
                                    from: 1
                                    to: Math.max(1, appController.reader.pageCount)
                                    value: appController.reader.currentPage + 1
                                    editable: true
                                    Layout.preferredWidth: 76
                                    onValueModified: appController.reader.goToPage(value - 1)
                                }
                                Label {
                                    text: qsTr("of %1").arg(appController.reader.pageCount)
                                    color: Theme.textSecondary
                                    font.pixelSize: 11
                                }
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
                                text: window.focusMode ? qsTr("Exit focus") : qsTr("Focus")
                                enabled: appController.reader.open
                                onClicked: window.focusMode = !window.focusMode
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Ctrl+Shift+F")
                            }
                            Button {
                                text: window.visibility === Window.FullScreen ? qsTr("Windowed") : qsTr("Full screen")
                                onClicked: window.toggleFullScreen()
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("F11")
                            }
                            Button {
                                visible: appController.reader.open && appController.reader.imageDocument
                                text: qsTr("Rotate")
                                onClicked: appController.reader.rotatePage(90)
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Rotate this page clockwise")
                            }
                            Button {
                                visible: appController.reader.open && appController.reader.pageCount > 1
                                text: appController.reader.imageDocument ? qsTr("Pages") : qsTr("Contents")
                                onClicked: pageDrawer.open()
                                ToolTip.visible: hovered
                                ToolTip.text: appController.reader.imageDocument
                                    ? qsTr("Open page thumbnails")
                                    : qsTr("Open document contents")
                            }
                            RowLayout {
                                visible: appController.reader.open && appController.reader.imageDocument
                                spacing: 2
                                ToolButton {
                                    text: "−"
                                    enabled: window.imageZoom > 0.5
                                    onClicked: window.changeImageZoom(-0.1)
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("Zoom out (Ctrl+-)")
                                }
                                Label {
                                    text: qsTr("%1%").arg(Math.round(window.imageZoom * 100))
                                    color: Theme.textSecondary
                                    horizontalAlignment: Text.AlignHCenter
                                    Layout.preferredWidth: 42
                                }
                                ToolButton {
                                    text: "+"
                                    enabled: window.imageZoom < 3.0
                                    onClicked: window.changeImageZoom(0.1)
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("Zoom in (Ctrl++)")
                                }
                                ToolButton {
                                    text: "1:1"
                                    enabled: Math.abs(window.imageZoom - 1.0) > 0.01
                                    onClicked: window.resetImageZoom()
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("Fit page (Ctrl+0)")
                                }
                            }
                            Button {
                                visible: appController.reader.open && appController.reader.imageDocument
                                text: appController.reader.showOcrText
                                    ? qsTr("Show page")
                                    : appController.reader.ocrRunning
                                        ? qsTr("Recognizing…")
                                        : qsTr("Read image")
                                enabled: !appController.reader.ocrRunning
                                onClicked: {
                                    if (appController.reader.showOcrText) {
                                        appController.reader.showOcrText = false
                                    } else if (!appController.reader.startOcr(
                                                   window.ocrLanguageKeys[ocrLanguageSelector.currentIndex])) {
                                        window.notification = appController.reader.ocrStatus
                                        notificationTimer.restart()
                                    }
                                }
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Recognize text without leaving the reader")
                            }
                            ComboBox {
                                id: ocrLanguageSelector
                                visible: appController.reader.open && appController.reader.imageDocument
                                enabled: !appController.reader.ocrRunning
                                Layout.preferredWidth: 112
                                model: window.ocrLanguageLabels
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("OCR language")
                            }
                            Label {
                                visible: appController.reader.open && appController.reader.ocrStatus.length > 0
                                text: appController.reader.ocrStatus
                                color: Theme.textSecondary
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                Layout.maximumWidth: 240
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
                                 text: qsTr("Bookmarks")
                                 enabled: appController.reader.open
                                 onClicked: bookmarksDialog.open()
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
                                                                    : (appController.settings.continuousReading
                                                                       && appController.reader.continuousTextContent.length > 0
                                                                       ? appController.reader.continuousTextContent
                                                                       : appController.reader.textContent))
                                     }
                                 }
                             }
                             Button {
                                 visible: appController.speech.speaking
                                 text: appController.speech.paused ? qsTr("Resume") : qsTr("Pause")
                                 enabled: appController.speech.available
                                 onClicked: appController.speech.paused
                                     ? appController.speech.resume()
                                     : appController.speech.pause()
                             }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Theme.readerBackground
                        clip: true

                        Image {
                            id: readerImage
                            anchors.fill: parent
                            anchors.margins: 28
                            visible: appController.reader.open && appController.reader.imageDocument
                                && !appController.reader.showOcrText
                            source: appController.reader.imageSource
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                            scale: window.imageZoom
                            transformOrigin: Item.Center
                            smooth: true
                        }

                        Item {
                            id: imageRegionLayer
                            anchors.fill: parent
                            visible: appController.reader.open && appController.reader.imageDocument
                                && !appController.reader.showOcrText
                            z: 2

                            property real startX: 0
                            property real startY: 0
                            property real currentX: 0
                            property real currentY: 0
                            property bool selecting: false
                            scale: window.imageZoom
                            transformOrigin: Item.Center

                            function clearSelection() {
                                selecting = false
                                startX = 0
                                startY = 0
                                currentX = 0
                                currentY = 0
                            }

                            function clamp(value, minimum, maximum) {
                                return Math.max(minimum, Math.min(maximum, value))
                            }

                            function openAnnotationForSelection() {
                                var left = Math.min(startX, currentX)
                                var top = Math.min(startY, currentY)
                                var right = Math.max(startX, currentX)
                                var bottom = Math.max(startY, currentY)
                                var imageLeft = readerImage.x
                                    + (readerImage.width - readerImage.paintedWidth) / 2
                                var imageTop = readerImage.y
                                    + (readerImage.height - readerImage.paintedHeight) / 2
                                var imageWidth = Math.max(readerImage.paintedWidth, 1)
                                var imageHeight = Math.max(readerImage.paintedHeight, 1)
                                var normalizedLeft = clamp((left - imageLeft) / imageWidth, 0, 1)
                                var normalizedTop = clamp((top - imageTop) / imageHeight, 0, 1)
                                var normalizedRight = clamp((right - imageLeft) / imageWidth, 0, 1)
                                var normalizedBottom = clamp((bottom - imageTop) / imageHeight, 0, 1)
                                var normalizedWidth = normalizedRight - normalizedLeft
                                var normalizedHeight = normalizedBottom - normalizedTop
                                if (normalizedWidth < 0.01 || normalizedHeight < 0.01) {
                                    clearSelection()
                                    return
                                }
                                regionAnnotationDialog.regionLeft = normalizedLeft
                                regionAnnotationDialog.regionTop = normalizedTop
                                regionAnnotationDialog.regionWidth = normalizedWidth
                                regionAnnotationDialog.regionHeight = normalizedHeight
                                regionAnnotationDialog.open()
                            }

                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton
                                onPressed: {
                                    imageRegionLayer.startX = mouse.x
                                    imageRegionLayer.startY = mouse.y
                                    imageRegionLayer.currentX = mouse.x
                                    imageRegionLayer.currentY = mouse.y
                                    imageRegionLayer.selecting = true
                                }
                                onPositionChanged: {
                                    if (imageRegionLayer.selecting) {
                                        imageRegionLayer.currentX = mouse.x
                                        imageRegionLayer.currentY = mouse.y
                                    }
                                }
                                onReleased: imageRegionLayer.openAnnotationForSelection()
                            }

                            Rectangle {
                                visible: imageRegionLayer.selecting
                                x: Math.min(imageRegionLayer.startX, imageRegionLayer.currentX)
                                y: Math.min(imageRegionLayer.startY, imageRegionLayer.currentY)
                                width: Math.abs(imageRegionLayer.currentX - imageRegionLayer.startX)
                                height: Math.abs(imageRegionLayer.currentY - imageRegionLayer.startY)
                                color: Qt.rgba(0.31, 0.27, 0.89, 0.18)
                                border.color: Theme.accent
                                border.width: 2
                            }
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
                                text: appController.settings.continuousReading
                                    && appController.reader.continuousTextContent.length > 0
                                    ? appController.reader.continuousTextContent
                                    : appController.reader.textContent
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
                                    ComboBox {
                                        id: selectionHighlightColor
                                        Layout.preferredWidth: 110
                                        model: [qsTr("Yellow"), qsTr("Green"), qsTr("Blue"), qsTr("Pink"), qsTr("Purple")]
                                    }
                                    ComboBox {
                                        id: selectionHighlightStyle
                                        Layout.preferredWidth: 120
                                        model: [qsTr("Highlight"), qsTr("Underline"), qsTr("Strikethrough")]
                                    }
                                    Button {
                                        text: qsTr("Highlight")
                                        onClicked: {
                                            appController.library.addHighlight(appController.reader.documentId,
                                                                               readerText.selectedText,
                                                                               "text:" + appController.reader.currentPage,
                                                                               appController.reader.currentPage,
                                                                               window.highlightColors[selectionHighlightColor.currentIndex],
                                                                               "",
                                                                               window.highlightStyles[selectionHighlightStyle.currentIndex])
                                            readerText.deselect()
                                            notification = qsTr("Passage saved to the Knowledge center.")
                                            notificationTimer.restart()
                                        }
                                    }
                                    Button {
                                        text: qsTr("Comment")
                                        onClicked: {
                                            textHighlightDialog.editingId = ""
                                            textHighlightDialog.selectedText = readerText.selectedText
                                            textHighlightDialog.selectedPage = appController.reader.currentPage
                                            textHighlightDialog.selectedLocator = "text:" + appController.reader.currentPage
                                            textHighlightColor.currentIndex = selectionHighlightColor.currentIndex
                                            textHighlightStyle.currentIndex = selectionHighlightStyle.currentIndex
                                            textHighlightDialog.open()
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
                                              lookupDialog.mode = "dictionary"
                                              lookupDialog.queryText = readerText.selectedText
                                              if (appController.lookup.lookupDictionary(readerText.selectedText)) {
                                                  lookupDialog.open()
                                              } else {
                                                  window.notification = appController.lookup.lastError
                                                  notificationTimer.restart()
                                              }
                                         }
                                     }
                                      ComboBox {
                                          id: selectionTranslationLanguage
                                          Layout.preferredWidth: 110
                                          model: window.translationLanguageLabels
                                          ToolTip.visible: hovered
                                          ToolTip.text: qsTr("Translation target language")
                                      }
                                     Button {
                                         text: qsTr("Translate")
                                         onClicked: {
                                              lookupDialog.mode = "translation"
                                              lookupDialog.queryText = readerText.selectedText
                                              if (appController.lookup.translateText(
                                                      readerText.selectedText,
                                                      "",
                                                      window.translationLanguageKeys[selectionTranslationLanguage.currentIndex])) {
                                                  lookupDialog.open()
                                              } else {
                                                  window.notification = appController.lookup.lastError
                                                  notificationTimer.restart()
                                              }
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

                            Label {
                                visible: appController.speech.available
                                text: qsTr("Read aloud")
                                color: Theme.textPrimary
                                font.weight: Font.DemiBold
                            }
                            RowLayout {
                                visible: appController.speech.available
                                Layout.fillWidth: true
                                Label { text: qsTr("Speed"); color: Theme.textPrimary; Layout.fillWidth: true }
                                Slider {
                                    Layout.preferredWidth: 190
                                    from: 50
                                    to: 200
                                    stepSize: 5
                                    value: appController.speech.rate
                                    onMoved: appController.speech.rate = Math.round(value)
                                }
                                Label { text: qsTr("%1%").arg(appController.speech.rate); color: Theme.textSecondary; Layout.preferredWidth: 44 }
                            }
                            RowLayout {
                                visible: appController.speech.available
                                Layout.fillWidth: true
                                Label { text: qsTr("Volume"); color: Theme.textPrimary; Layout.fillWidth: true }
                                Slider {
                                    Layout.preferredWidth: 190
                                    from: 0
                                    to: 100
                                    stepSize: 5
                                    value: appController.speech.volume
                                    onMoved: appController.speech.volume = Math.round(value)
                                }
                                Label { text: qsTr("%1%").arg(appController.speech.volume); color: Theme.textSecondary; Layout.preferredWidth: 44 }
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
                        Button {
                            text: qsTr("Export preferences")
                            onClicked: settingsExportDialog.open()
                        }
                        Button {
                            text: qsTr("Restore preferences")
                            onClicked: settingsRestoreDialog.open()
                        }
                        Button {
                            text: qsTr("New collection")
                            onClicked: createCollectionDialog.open()
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

#pragma once

#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QList>
#include <QSqlQuery>
#include <QVariant>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace Lumina::Infrastructure {
class Database;

class LibraryStore final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(QString viewFilter READ viewFilter WRITE setViewFilter NOTIFY viewFilterChanged)
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QVariantList searchHistory READ searchHistory NOTIFY searchHistoryChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        AuthorRole,
        FilePathRole,
        FormatRole,
        ContentTypeRole,
        StatusRole,
        ProgressRole,
        CurrentPageRole,
        PageCountRole,
        CoverPathRole,
        FileSizeRole,
        AddedAtRole,
        LastOpenedAtRole,
        DescriptionRole,
        LanguageRole,
        FavoriteRole,
        RatingRole,
        PageRotationRole,
        ReadingSecondsRole,
        GenreRole,
    };
    Q_ENUM(Role)

    explicit LibraryStore(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString query() const;
    void setQuery(const QString &query);
    [[nodiscard]] QString viewFilter() const;
    void setViewFilter(const QString &filter);
    [[nodiscard]] QString sortMode() const;
    void setSortMode(const QString &mode);
    [[nodiscard]] int count() const;
    [[nodiscard]] QVariantList searchHistory() const;
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;

    void setDatabase(Database *database);
    Q_INVOKABLE void refresh();

    Q_INVOKABLE bool importFiles(const QStringList &filePaths);
    Q_INVOKABLE bool importFolder(const QString &folderPath, bool recursive = true);
    Q_INVOKABLE bool watchFolder(const QString &folderPath, bool recursive = true);
    Q_INVOKABLE bool unwatchFolder(const QString &folderPath);
    Q_INVOKABLE bool documentAvailable(const QString &documentId) const;
    Q_INVOKABLE bool relinkDocument(const QString &documentId, const QString &filePath);
    Q_INVOKABLE bool removeDocument(const QString &documentId, bool deleteOriginal = false);
    Q_INVOKABLE bool setStatus(const QString &documentId, const QString &status);
    Q_INVOKABLE bool setPageCount(const QString &documentId, int pageCount);
    Q_INVOKABLE bool setFavorite(const QString &documentId, bool favorite);
    Q_INVOKABLE bool setRating(const QString &documentId, int rating);
    Q_INVOKABLE bool updateMetadata(const QString &documentId,
                                    const QString &title,
                                    const QString &author,
                                    const QString &description,
                                    const QString &language,
                                    const QString &contentType = {},
                                    const QString &genre = {});
    Q_INVOKABLE bool setCoverPath(const QString &documentId, const QString &coverPath);
    Q_INVOKABLE bool setPageRotation(const QString &documentId, int rotation);
    Q_INVOKABLE bool updateProgress(const QString &documentId, double progress, int currentPage);
    bool addReadingTime(const QString &documentId, qint64 seconds);
    Q_INVOKABLE QVariantMap documentDetails(const QString &documentId) const;
    Q_INVOKABLE QVariantList collections() const;
    Q_INVOKABLE QVariantList collectionsForDocument(const QString &documentId) const;
    Q_INVOKABLE bool createCollection(const QString &name, const QString &description = {});
    Q_INVOKABLE bool deleteCollection(const QString &collectionId);
    Q_INVOKABLE bool addToCollection(const QString &documentId, const QString &collectionId);
    Q_INVOKABLE bool removeFromCollection(const QString &documentId, const QString &collectionId);
    Q_INVOKABLE QVariantList tags() const;
    Q_INVOKABLE QStringList tagsForDocument(const QString &documentId) const;
    Q_INVOKABLE QStringList genres() const;
    Q_INVOKABLE QStringList languages() const;
    Q_INVOKABLE QStringList formats() const;
    Q_INVOKABLE bool setTags(const QString &documentId, const QStringList &tags);
    Q_INVOKABLE QVariantList knowledgeDocumentOptions() const;
    Q_INVOKABLE QVariantList searchHistoryForScope(const QString &scope) const;
    Q_INVOKABLE bool recordSearch(const QString &query, const QString &scope);
    Q_INVOKABLE bool clearSearchHistory(const QString &scope = {});
    Q_INVOKABLE bool undo();
    Q_INVOKABLE bool redo();
    Q_INVOKABLE QVariantList knowledgeSearch(const QString &term,
                                             const QString &format = {},
                                             const QString &color = {},
                                             const QString &documentId = {},
                                             const QString &author = {},
                                             const QString &createdAfter = {},
                                             const QString &createdBefore = {},
                                             const QString &order = {}) const;
    Q_INVOKABLE QVariantList highlightsForDocument(const QString &documentId) const;
    Q_INVOKABLE bool removeHighlight(const QString &highlightId);
    Q_INVOKABLE bool addHighlight(const QString &documentId,
                                  const QString &text,
                                  const QString &locator,
                                  int page,
                                  const QString &color,
                                  const QString &note = {},
                                  const QString &style = QStringLiteral("highlight"));
    Q_INVOKABLE bool updateHighlight(const QString &highlightId,
                                     const QString &color,
                                     const QString &note,
                                     const QString &style);
    Q_INVOKABLE QVariantList annotationsForDocument(const QString &documentId) const;
    Q_INVOKABLE bool addAnnotation(const QString &documentId,
                                   const QString &text,
                                   const QString &kind,
                                   const QString &locator,
                                   int page,
                                   double x = 0.0,
                                   double y = 0.0,
                                   double width = 0.0,
                                   double height = 0.0);
    Q_INVOKABLE bool updateAnnotation(const QString &annotationId,
                                      const QString &text,
                                      const QString &kind);
    Q_INVOKABLE bool removeAnnotation(const QString &annotationId);
    [[nodiscard]] QString ocrTextForPage(const QString &documentId, int page) const;
    bool saveOcrPage(const QString &documentId,
                     int page,
                     const QString &language,
                     const QString &text,
                     const QString &status = QStringLiteral("completed"));
    Q_INVOKABLE QVariantList bookmarksForDocument(const QString &documentId) const;
    Q_INVOKABLE bool addBookmark(const QString &documentId,
                                 const QString &label,
                                 const QString &locator,
                                 int page);
    Q_INVOKABLE bool removeBookmark(const QString &bookmarkId);
    Q_INVOKABLE QStringList supportedExtensions() const;

signals:
    void queryChanged();
    void viewFilterChanged();
    void sortModeChanged();
    void countChanged();
    void searchHistoryChanged();
    void historyChanged();
    void libraryChanged();
    void documentsImported(int count);
    void errorOccurred(const QString &message);

private:
    bool importSingle(const QString &filePath);
    bool isReady() const;
    void reload();
    void setError(const QString &message);
    [[nodiscard]] QVariantMap recordFromQuery(const QSqlQuery &query) const;

    struct KnowledgeAction {
        QString kind;
        QString operation;
        QVariantMap before;
        QVariantMap after;
    };

    void pushKnowledgeAction(const QString &kind,
                             const QString &operation,
                             const QVariantMap &before,
                             const QVariantMap &after);
    [[nodiscard]] bool applyKnowledgeAction(const KnowledgeAction &action, bool forward);
    [[nodiscard]] bool restoreHighlight(const QVariantMap &record);
    [[nodiscard]] bool deleteHighlightRecord(const QString &highlightId);
    [[nodiscard]] bool updateHighlightRecord(const QVariantMap &record);
    [[nodiscard]] bool restoreAnnotation(const QVariantMap &record);
    [[nodiscard]] bool deleteAnnotationRecord(const QString &annotationId);
    [[nodiscard]] bool updateAnnotationRecord(const QVariantMap &record);

    Database *database_ = nullptr;
    QList<QVariantMap> records_;
    QString query_;
    QString viewFilter_;
    QString sortMode_ = QStringLiteral("recent");
    QFileSystemWatcher *folderWatcher_ = nullptr;
    QStringList watchedFolders_;
    QList<KnowledgeAction> undoStack_;
    QList<KnowledgeAction> redoStack_;
};

} // namespace Lumina::Infrastructure

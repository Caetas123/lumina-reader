#pragma once

#include <QAbstractListModel>
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
    Q_PROPERTY(int count READ count NOTIFY countChanged)

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
    };
    Q_ENUM(Role)

    explicit LibraryStore(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString query() const;
    void setQuery(const QString &query);
    [[nodiscard]] int count() const;

    void setDatabase(Database *database);

    Q_INVOKABLE bool importFiles(const QStringList &filePaths);
    Q_INVOKABLE bool removeDocument(const QString &documentId, bool deleteOriginal = false);
    Q_INVOKABLE bool setStatus(const QString &documentId, const QString &status);
    Q_INVOKABLE bool updateProgress(const QString &documentId, double progress, int currentPage);
    Q_INVOKABLE QVariantMap documentDetails(const QString &documentId) const;
    Q_INVOKABLE QVariantList knowledgeSearch(const QString &term) const;
    Q_INVOKABLE bool addHighlight(const QString &documentId,
                                  const QString &text,
                                  const QString &locator,
                                  int page,
                                  const QString &color,
                                  const QString &note = {});
    Q_INVOKABLE QStringList supportedExtensions() const;

signals:
    void queryChanged();
    void countChanged();
    void libraryChanged();
    void documentsImported(int count);
    void errorOccurred(const QString &message);

private:
    bool importSingle(const QString &filePath);
    bool isReady() const;
    void reload();
    void setError(const QString &message);
    [[nodiscard]] QVariantMap recordFromQuery(const QSqlQuery &query) const;

    Database *database_ = nullptr;
    QList<QVariantMap> records_;
    QString query_;
};

} // namespace Lumina::Infrastructure

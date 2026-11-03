#include "infrastructure/library/LibraryStore.h"

#include "infrastructure/database/Database.h"
#include "infrastructure/archive/ArchiveReader.h"
#include "infrastructure/logging/Logging.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

#include <algorithm>

namespace {

QString normalizedPath(const QString &path)
{
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? info.absoluteFilePath() : canonical;
}

QString formatForPath(const QString &path)
{
    return QFileInfo(path).suffix().toLower();
}

QStringList supportedFormats()
{
    return {
        QStringLiteral("pdf"),   QStringLiteral("epub"), QStringLiteral("mobi"),
        QStringLiteral("azw"),   QStringLiteral("azw3"), QStringLiteral("fb2"),
        QStringLiteral("txt"),   QStringLiteral("rtf"),  QStringLiteral("cbz"),
        QStringLiteral("cbr"),   QStringLiteral("cb7"),  QStringLiteral("cbt"),
        QStringLiteral("zip"),   QStringLiteral("rar"),  QStringLiteral("7z"),
        QStringLiteral("jpg"),   QStringLiteral("jpeg"), QStringLiteral("png"),
        QStringLiteral("webp"),  QStringLiteral("avif"), QStringLiteral("gif"),
        QStringLiteral("djvu"),  QStringLiteral("djv"),
    };
}

bool isImageFormat(const QString &format)
{
    return format == QStringLiteral("jpg") || format == QStringLiteral("jpeg")
        || format == QStringLiteral("png") || format == QStringLiteral("webp")
        || format == QStringLiteral("avif") || format == QStringLiteral("gif");
}

bool isComicFormat(const QString &format)
{
    return format == QStringLiteral("cbz") || format == QStringLiteral("cbr")
        || format == QStringLiteral("cb7") || format == QStringLiteral("cbt")
        || format == QStringLiteral("zip") || format == QStringLiteral("rar")
        || format == QStringLiteral("7z");
}

QString contentTypeForFormat(const QString &format)
{
    if (isImageFormat(format)) {
        return QStringLiteral("image");
    }
    if (isComicFormat(format)) {
        return QStringLiteral("comic");
    }
    if (format == QStringLiteral("epub") || format == QStringLiteral("mobi")
        || format == QStringLiteral("azw") || format == QStringLiteral("azw3")) {
        return QStringLiteral("book");
    }
    if (format == QStringLiteral("pdf") || format == QStringLiteral("djvu")
        || format == QStringLiteral("djv")) {
        return QStringLiteral("document");
    }
    return QStringLiteral("text");
}

bool hasSupportedFormat(const QString &format)
{
    return supportedFormats().contains(format);
}

bool hasExpectedSignature(const QString &path, const QString &format)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray header = file.read(12);
    if (isImageFormat(format)) {
        QImageReader reader(path);
        return reader.canRead();
    }
    if (format == QStringLiteral("pdf")) {
        return header.startsWith("%PDF-");
    }
    if (format == QStringLiteral("rar")) {
        return header.startsWith("Rar!\x1A\x07");
    }
    if (format == QStringLiteral("7z")) {
        return header.startsWith("7z\xBC\xAF\x27\x1C");
    }
    if (format == QStringLiteral("epub") || format == QStringLiteral("cbz")
        || format == QStringLiteral("zip")) {
        return header.startsWith("PK\x03\x04") || header.startsWith("PK\x05\x06")
            || header.startsWith("PK\x07\x08");
    }
    return !header.isEmpty();
}

QString calculateHash(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        hash.addData(file.read(1024 * 1024));
    }
    return QString::fromLatin1(hash.result().toHex());
}

QString searchableText(const QString &path, const QString &format)
{
    if (format == QStringLiteral("epub")) {
        Lumina::Infrastructure::ArchiveReader archive;
        if (!archive.open(path, format)) {
            return {};
        }

        QString text;
        for (int page = 0; page < archive.pageCount(); ++page) {
            text.append(archive.pageText(page));
            text.append(QChar('\n'));
            if (text.size() >= 4 * 1024 * 1024) {
                break;
            }
        }
        return text.left(4 * 1024 * 1024);
    }

    if (format != QStringLiteral("txt") && format != QStringLiteral("rtf")
        && format != QStringLiteral("fb2")) {
        return {};
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    constexpr qint64 maximumIndexedBytes = 4 * 1024 * 1024;
    return QString::fromUtf8(file.read(maximumIndexedBytes));
}

QString ftsPrefixQuery(const QString &value)
{
    QStringList tokens;
    const QRegularExpression tokenExpression(QStringLiteral(R"([\p{L}\p{N}_]+)"));
    QRegularExpressionMatchIterator iterator = tokenExpression.globalMatch(value);
    while (iterator.hasNext()) {
        QString token = iterator.next().captured(0);
        token.replace(QStringLiteral("\""), QStringLiteral("\"\""));
        tokens.append(QStringLiteral("\"") + token + QStringLiteral("\"*"));
    }
    return tokens.join(QStringLiteral(" AND "));
}

void setError(Lumina::Domain::Error *error,
              Lumina::Domain::ErrorCode code,
              const QString &userMessage,
              const QString &technicalMessage)
{
    if (error != nullptr) {
        *error = Lumina::Domain::Error::create(code, userMessage, technicalMessage);
    }
}

} // namespace

namespace Lumina::Infrastructure {

LibraryStore::LibraryStore(QObject *parent)
    : QAbstractListModel(parent)
{
}

int LibraryStore::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : records_.size();
}

QVariant LibraryStore::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= records_.size()) {
        return {};
    }

    const QVariantMap &record = records_.at(index.row());
    const QByteArray roleName = roleNames().value(role);
    return record.value(QString::fromLatin1(roleName));
}

QHash<int, QByteArray> LibraryStore::roleNames() const
{
    return {
        {IdRole, "id"},
        {TitleRole, "title"},
        {AuthorRole, "author"},
        {FilePathRole, "filePath"},
        {FormatRole, "format"},
        {ContentTypeRole, "contentType"},
        {StatusRole, "status"},
        {ProgressRole, "progress"},
        {CurrentPageRole, "currentPage"},
        {PageCountRole, "pageCount"},
        {CoverPathRole, "coverPath"},
        {FileSizeRole, "fileSize"},
        {AddedAtRole, "addedAt"},
        {LastOpenedAtRole, "lastOpenedAt"},
    };
}

QString LibraryStore::query() const
{
    return query_;
}

void LibraryStore::setQuery(const QString &query)
{
    const QString trimmed = query.trimmed();
    if (trimmed == query_) {
        return;
    }
    query_ = trimmed;
    emit queryChanged();
    reload();
}

int LibraryStore::count() const
{
    return records_.size();
}

void LibraryStore::setDatabase(Database *database)
{
    if (database_ == database) {
        return;
    }
    database_ = database;
    reload();
}

bool LibraryStore::isReady() const
{
    return database_ != nullptr && database_->isOpen();
}

void LibraryStore::setError(const QString &message)
{
    qCWarning(luminaDatabase) << message;
    emit errorOccurred(message);
}

void LibraryStore::reload()
{
    beginResetModel();
    records_.clear();

    if (!isReady()) {
        endResetModel();
        emit countChanged();
        return;
    }

    QSqlQuery query(database_->connection());
    const QString columns = QStringLiteral(
        "d.id, d.title, d.author, d.file_path, d.format, d.content_type, d.status, "
        "d.progress, d.current_page, d.page_count, d.cover_path, d.file_size, "
        "d.added_at, COALESCE(d.last_opened_at, '')");
    if (query_.isEmpty()) {
        query.prepare(QStringLiteral("SELECT %1 FROM documents d "
                                     "ORDER BY COALESCE(d.last_opened_at, d.added_at) DESC, d.title COLLATE NOCASE")
                          .arg(columns));
    } else {
        query.prepare(QStringLiteral(
                          "SELECT %1 FROM documents d "
                          "JOIN document_search ON document_search.document_id = d.id "
                          "WHERE document_search MATCH :match "
                          "ORDER BY rank")
                          .arg(columns));
        query.bindValue(QStringLiteral(":match"), ftsPrefixQuery(query_));
    }

    if (!query.exec()) {
        if (!query_.isEmpty()) {
            QSqlQuery fallback(database_->connection());
            fallback.prepare(QStringLiteral(
                "SELECT %1 FROM documents d WHERE d.title LIKE :term OR d.author LIKE :term "
                "OR d.description LIKE :term ORDER BY COALESCE(d.last_opened_at, d.added_at) DESC")
                                  .arg(columns));
            fallback.bindValue(QStringLiteral(":term"), QStringLiteral("%") + query_ + QStringLiteral("%"));
            if (fallback.exec()) {
                while (fallback.next()) {
                    records_.append(recordFromQuery(fallback));
                }
            }
        } else {
            setError(QStringLiteral("The library could not be loaded."));
        }
    } else {
        while (query.next()) {
            records_.append(recordFromQuery(query));
        }
    }

    endResetModel();
    emit countChanged();
    emit libraryChanged();
}

QVariantMap LibraryStore::recordFromQuery(const QSqlQuery &query) const
{
    return {
        {QStringLiteral("id"), query.value(0)},
        {QStringLiteral("title"), query.value(1)},
        {QStringLiteral("author"), query.value(2)},
        {QStringLiteral("filePath"), query.value(3)},
        {QStringLiteral("format"), query.value(4)},
        {QStringLiteral("contentType"), query.value(5)},
        {QStringLiteral("status"), query.value(6)},
        {QStringLiteral("progress"), query.value(7)},
        {QStringLiteral("currentPage"), query.value(8)},
        {QStringLiteral("pageCount"), query.value(9)},
        {QStringLiteral("coverPath"), query.value(10)},
        {QStringLiteral("fileSize"), query.value(11)},
        {QStringLiteral("addedAt"), query.value(12)},
        {QStringLiteral("lastOpenedAt"), query.value(13)},
    };
}

bool LibraryStore::importFiles(const QStringList &filePaths)
{
    if (!isReady()) {
        setError(QStringLiteral("The local library is not ready yet."));
        return false;
    }

    int imported = 0;
    for (const QString &filePath : filePaths) {
        if (importSingle(filePath)) {
            ++imported;
        }
    }

    if (imported > 0) {
        reload();
        emit documentsImported(imported);
    }
    return imported > 0;
}

bool LibraryStore::importSingle(const QString &filePath)
{
    const QFileInfo info(filePath);
    const QString format = formatForPath(filePath);
    if (!info.exists() || !info.isFile() || !info.isReadable()) {
        setError(QStringLiteral("The selected file could not be read."));
        return false;
    }
    if (!hasSupportedFormat(format)) {
        setError(QStringLiteral("This file format is not supported by Lumina Reader yet."));
        return false;
    }
    if (!hasExpectedSignature(filePath, format)) {
        setError(QStringLiteral("The selected file does not match its file extension."));
        return false;
    }

    const QString path = normalizedPath(filePath);
    const QString hash = calculateHash(path);
    if (hash.isEmpty()) {
        setError(QStringLiteral("The selected file could not be inspected."));
        return false;
    }

    QSqlDatabase connection = database_->connection();
    QSqlQuery existing(connection);
    existing.prepare(QStringLiteral("SELECT id, file_path FROM documents WHERE file_hash = :hash"));
    existing.bindValue(QStringLiteral(":hash"), hash);
    if (!existing.exec()) {
        setError(QStringLiteral("The library could not check for duplicate files."));
        return false;
    }
    if (existing.next()) {
        QSqlQuery update(connection);
        update.prepare(QStringLiteral(
            "UPDATE documents SET file_path = :path, canonical_path = :canonical, "
            "updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
        update.bindValue(QStringLiteral(":path"), path);
        update.bindValue(QStringLiteral(":canonical"), path);
        update.bindValue(QStringLiteral(":id"), existing.value(0));
        if (!update.exec()) {
            setError(QStringLiteral("The duplicate document location could not be updated."));
            return false;
        }
        return true;
    }

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString title = info.completeBaseName().trimmed().isEmpty()
        ? info.fileName()
        : info.completeBaseName();
    const QString contentType = contentTypeForFormat(format);
    const QString text = searchableText(path, format);

    if (!connection.transaction()) {
        setError(QStringLiteral("The document could not be added to the library."));
        return false;
    }

    QSqlQuery insert(connection);
    insert.prepare(QStringLiteral(
        "INSERT INTO documents (id, file_path, canonical_path, file_hash, file_size, format, "
        "content_type, title) VALUES (:id, :path, :canonical, :hash, :size, :format, :type, :title)"));
    insert.bindValue(QStringLiteral(":id"), id);
    insert.bindValue(QStringLiteral(":path"), path);
    insert.bindValue(QStringLiteral(":canonical"), path);
    insert.bindValue(QStringLiteral(":hash"), hash);
    insert.bindValue(QStringLiteral(":size"), info.size());
    insert.bindValue(QStringLiteral(":format"), format);
    insert.bindValue(QStringLiteral(":type"), contentType);
    insert.bindValue(QStringLiteral(":title"), title);
    bool success = insert.exec();

    if (success) {
        QSqlQuery search(connection);
        search.prepare(QStringLiteral(
            "INSERT INTO document_search (document_id, title, author, description, content) "
            "VALUES (:id, :title, '', '', :content)"));
        search.bindValue(QStringLiteral(":id"), id);
        search.bindValue(QStringLiteral(":title"), title);
        search.bindValue(QStringLiteral(":content"), text);
        success = search.exec();
    }

    if (!success || !connection.commit()) {
        connection.rollback();
        setError(QStringLiteral("The document could not be added to the library."));
        return false;
    }

    qCInfo(luminaDatabase) << "Imported document" << id << format;
    return true;
}

bool LibraryStore::removeDocument(const QString &documentId, bool deleteOriginal)
{
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return false;
    }

    const QVariantMap record = documentDetails(documentId);
    if (record.isEmpty()) {
        return false;
    }

    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        setError(QStringLiteral("The document could not be removed from the library."));
        return false;
    }

    QSqlQuery knowledgeSearch(connection);
    knowledgeSearch.prepare(QStringLiteral("DELETE FROM knowledge_search WHERE document_id = :id"));
    knowledgeSearch.bindValue(QStringLiteral(":id"), documentId);
    if (!knowledgeSearch.exec()) {
        connection.rollback();
        setError(QStringLiteral("The document could not be removed from knowledge search."));
        return false;
    }

    QSqlQuery knowledgeItemsSearch(connection);
    knowledgeItemsSearch.prepare(
        QStringLiteral("DELETE FROM knowledge_items_search WHERE document_id = :id"));
    knowledgeItemsSearch.bindValue(QStringLiteral(":id"), documentId);
    if (!knowledgeItemsSearch.exec()) {
        connection.rollback();
        setError(QStringLiteral("The document could not be removed from knowledge search."));
        return false;
    }

    QSqlQuery search(connection);
    search.prepare(QStringLiteral("DELETE FROM document_search WHERE document_id = :id"));
    search.bindValue(QStringLiteral(":id"), documentId);
    if (!search.exec()) {
        connection.rollback();
        setError(QStringLiteral("The document could not be removed from search."));
        return false;
    }

    QSqlQuery query(connection);
    query.prepare(QStringLiteral("DELETE FROM documents WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec()) {
        connection.rollback();
        setError(QStringLiteral("The document could not be removed from the library."));
        return false;
    }
    if (!connection.commit()) {
        connection.rollback();
        setError(QStringLiteral("The document could not be removed from the library."));
        return false;
    }

    if (deleteOriginal && !QFile::remove(record.value(QStringLiteral("filePath")).toString())) {
        setError(QStringLiteral("The original file could not be deleted."));
    }
    reload();
    return true;
}

bool LibraryStore::setStatus(const QString &documentId, const QString &status)
{
    static const QStringList validStatuses = {
        QStringLiteral("to_read"), QStringLiteral("reading"), QStringLiteral("read"),
        QStringLiteral("abandoned"),
    };
    if (!isReady() || !validStatuses.contains(status)) {
        return false;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "UPDATE documents SET status = :status, updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
    query.bindValue(QStringLiteral(":status"), status);
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec() || query.numRowsAffected() == 0) {
        return false;
    }
    reload();
    return true;
}

bool LibraryStore::setPageCount(const QString &documentId, int pageCount)
{
    if (!isReady() || documentId.trimmed().isEmpty() || pageCount <= 0) {
        return false;
    }

    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "UPDATE documents SET page_count = :page_count, updated_at = CURRENT_TIMESTAMP "
        "WHERE id = :id"));
    query.bindValue(QStringLiteral(":page_count"), pageCount);
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec() || query.numRowsAffected() == 0) {
        return false;
    }
    reload();
    return true;
}

bool LibraryStore::updateProgress(const QString &documentId, double progress, int currentPage)
{
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return false;
    }

    progress = std::clamp(progress, 0.0, 1.0);
    currentPage = std::max(currentPage, 0);
    const QString status = progress >= 0.999 ? QStringLiteral("read")
                                             : progress > 0.0 ? QStringLiteral("reading")
                                                              : QStringLiteral("to_read");

    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }
    QSqlQuery update(connection);
    update.prepare(QStringLiteral(
        "UPDATE documents SET progress = :progress, current_page = :page, status = :status, "
        "last_opened_at = CURRENT_TIMESTAMP, updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
    update.bindValue(QStringLiteral(":progress"), progress);
    update.bindValue(QStringLiteral(":page"), currentPage);
    update.bindValue(QStringLiteral(":status"), status);
    update.bindValue(QStringLiteral(":id"), documentId);
    bool success = update.exec() && update.numRowsAffected() > 0;
    if (success) {
        QSqlQuery history(connection);
        history.prepare(QStringLiteral(
            "INSERT INTO reading_history (document_id, progress, page) "
            "VALUES (:id, :progress, :page)"));
        history.bindValue(QStringLiteral(":id"), documentId);
        history.bindValue(QStringLiteral(":progress"), progress);
        history.bindValue(QStringLiteral(":page"), currentPage);
        success = history.exec();
    }
    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    reload();
    return true;
}

QVariantMap LibraryStore::documentDetails(const QString &documentId) const
{
    if (!isReady()) {
        return {};
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "SELECT id, title, author, file_path, format, content_type, status, progress, "
        "current_page, page_count, cover_path, file_size, added_at, COALESCE(last_opened_at, '') "
        "FROM documents WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec() || !query.next()) {
        return {};
    }
    return recordFromQuery(query);
}

QVariantList LibraryStore::knowledgeSearch(const QString &term) const
{
    QVariantList results;
    if (!isReady() || term.trimmed().isEmpty()) {
        return results;
    }

    const QString match = ftsPrefixQuery(term);
    if (match.isEmpty()) {
        return results;
    }

    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "SELECT ks.source_id, ks.source_kind, ks.document_id, d.title, d.author, ks.text, "
        "COALESCE(h.note, ''), COALESCE(h.color, '#f6c945'), ks.page, ks.locator, "
        "ks.created_at, COALESCE(a.kind, '') "
        "FROM knowledge_items_search ks JOIN documents d ON d.id = ks.document_id "
        "LEFT JOIN highlights h ON h.id = ks.source_id AND ks.source_kind = 'highlight' "
        "LEFT JOIN annotations a ON a.id = ks.source_id AND ks.source_kind = 'annotation' "
        "WHERE knowledge_items_search MATCH :match ORDER BY rank, ks.created_at DESC"));
    query.bindValue(QStringLiteral(":match"), match);
    if (!query.exec()) {
        return results;
    }
    while (query.next()) {
        results.append(QVariantMap{
            {QStringLiteral("id"), query.value(0)},
            {QStringLiteral("kind"), query.value(1)},
            {QStringLiteral("documentId"), query.value(2)},
            {QStringLiteral("documentTitle"), query.value(3)},
            {QStringLiteral("author"), query.value(4)},
            {QStringLiteral("text"), query.value(5)},
            {QStringLiteral("note"), query.value(6)},
            {QStringLiteral("color"), query.value(7)},
            {QStringLiteral("page"), query.value(8)},
            {QStringLiteral("locator"), query.value(9)},
            {QStringLiteral("createdAt"), query.value(10)},
            {QStringLiteral("annotationKind"), query.value(11)},
        });
    }
    return results;
}

bool LibraryStore::addHighlight(const QString &documentId,
                                const QString &text,
                                const QString &locator,
                                int page,
                                const QString &color,
                                const QString &note)
{
    if (!isReady() || documentDetails(documentId).isEmpty() || text.trimmed().isEmpty()
        || locator.trimmed().isEmpty()) {
        return false;
    }
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }

    QSqlQuery query(connection);
    query.prepare(QStringLiteral(
        "INSERT INTO highlights (id, document_id, text, note, color, locator, page) "
        "VALUES (:id, :document, :text, :note, :color, :locator, :page)"));
    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":document"), documentId);
    query.bindValue(QStringLiteral(":text"), text);
    query.bindValue(QStringLiteral(":note"), note);
    query.bindValue(QStringLiteral(":color"), color.isEmpty() ? QStringLiteral("#f6c945") : color);
    query.bindValue(QStringLiteral(":locator"), locator);
    query.bindValue(QStringLiteral(":page"), std::max(page, 0));
    bool success = query.exec();
    if (success) {
        QSqlQuery search(connection);
        search.prepare(QStringLiteral(
            "INSERT INTO knowledge_search (highlight_id, document_id, text, note) "
            "VALUES (:highlight, :document, :text, :note)"));
        search.bindValue(QStringLiteral(":highlight"), id);
        search.bindValue(QStringLiteral(":document"), documentId);
        search.bindValue(QStringLiteral(":text"), text);
        search.bindValue(QStringLiteral(":note"), note);
        success = search.exec();
    }
    if (success) {
        QSqlQuery search(connection);
        search.prepare(QStringLiteral(
            "INSERT INTO knowledge_items_search "
            "(source_id, source_kind, document_id, text, note, page, locator, created_at) "
            "VALUES (:source, 'highlight', :document, :text, :note, :page, :locator, CURRENT_TIMESTAMP)"));
        search.bindValue(QStringLiteral(":source"), id);
        search.bindValue(QStringLiteral(":document"), documentId);
        search.bindValue(QStringLiteral(":text"), text);
        search.bindValue(QStringLiteral(":note"), note);
        search.bindValue(QStringLiteral(":page"), std::max(page, 0));
        search.bindValue(QStringLiteral(":locator"), locator);
        success = search.exec();
    }
    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    return true;
}

QVariantList LibraryStore::highlightsForDocument(const QString &documentId) const
{
    QVariantList results;
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return results;
    }

    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "SELECT id, text, note, color, style, locator, context_before, context_after, page, "
        "created_at, updated_at FROM highlights WHERE document_id = :id ORDER BY page, created_at"));
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec()) {
        return results;
    }
    while (query.next()) {
        results.append(QVariantMap{
            {QStringLiteral("id"), query.value(0)},
            {QStringLiteral("text"), query.value(1)},
            {QStringLiteral("note"), query.value(2)},
            {QStringLiteral("color"), query.value(3)},
            {QStringLiteral("style"), query.value(4)},
            {QStringLiteral("locator"), query.value(5)},
            {QStringLiteral("contextBefore"), query.value(6)},
            {QStringLiteral("contextAfter"), query.value(7)},
            {QStringLiteral("page"), query.value(8)},
            {QStringLiteral("createdAt"), query.value(9)},
            {QStringLiteral("updatedAt"), query.value(10)},
        });
    }
    return results;
}

bool LibraryStore::removeHighlight(const QString &highlightId)
{
    if (!isReady() || highlightId.trimmed().isEmpty()) {
        return false;
    }
    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }
    QSqlQuery search(connection);
    search.prepare(QStringLiteral(
        "DELETE FROM knowledge_items_search WHERE source_id = :id AND source_kind = 'highlight'"));
    search.bindValue(QStringLiteral(":id"), highlightId);
    bool success = search.exec();
    if (success) {
        QSqlQuery legacy(connection);
        legacy.prepare(QStringLiteral("DELETE FROM knowledge_search WHERE highlight_id = :id"));
        legacy.bindValue(QStringLiteral(":id"), highlightId);
        success = legacy.exec();
    }
    if (success) {
        QSqlQuery query(connection);
        query.prepare(QStringLiteral("DELETE FROM highlights WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), highlightId);
        success = query.exec() && query.numRowsAffected() > 0;
    }
    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    return true;
}

QVariantList LibraryStore::annotationsForDocument(const QString &documentId) const
{
    QVariantList results;
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return results;
    }

    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "SELECT id, text, kind, locator, page, x, y, width, height, created_at, updated_at "
        "FROM annotations WHERE document_id = :id ORDER BY page, created_at"));
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec()) {
        return results;
    }
    while (query.next()) {
        results.append(QVariantMap{
            {QStringLiteral("id"), query.value(0)},
            {QStringLiteral("text"), query.value(1)},
            {QStringLiteral("kind"), query.value(2)},
            {QStringLiteral("locator"), query.value(3)},
            {QStringLiteral("page"), query.value(4)},
            {QStringLiteral("x"), query.value(5)},
            {QStringLiteral("y"), query.value(6)},
            {QStringLiteral("width"), query.value(7)},
            {QStringLiteral("height"), query.value(8)},
            {QStringLiteral("createdAt"), query.value(9)},
            {QStringLiteral("updatedAt"), query.value(10)},
        });
    }
    return results;
}

bool LibraryStore::addAnnotation(const QString &documentId,
                                 const QString &text,
                                 const QString &kind,
                                 const QString &locator,
                                 int page,
                                 double x,
                                 double y,
                                 double width,
                                 double height)
{
    if (!isReady() || documentDetails(documentId).isEmpty() || text.trimmed().isEmpty()
        || locator.trimmed().isEmpty()) {
        return false;
    }
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }
    QSqlQuery query(connection);
    query.prepare(QStringLiteral(
        "INSERT INTO annotations (id, document_id, text, kind, locator, page, x, y, width, height) "
        "VALUES (:id, :document, :text, :kind, :locator, :page, :x, :y, :width, :height)"));
    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":document"), documentId);
    query.bindValue(QStringLiteral(":text"), text);
    query.bindValue(QStringLiteral(":kind"), kind.isEmpty() ? QStringLiteral("note") : kind);
    query.bindValue(QStringLiteral(":locator"), locator);
    query.bindValue(QStringLiteral(":page"), std::max(page, 0));
    query.bindValue(QStringLiteral(":x"), x);
    query.bindValue(QStringLiteral(":y"), y);
    query.bindValue(QStringLiteral(":width"), width);
    query.bindValue(QStringLiteral(":height"), height);
    bool success = query.exec();
    if (success) {
        QSqlQuery search(connection);
        search.prepare(QStringLiteral(
            "INSERT INTO knowledge_items_search "
            "(source_id, source_kind, document_id, text, note, page, locator, created_at) "
            "VALUES (:source, 'annotation', :document, :text, :note, :page, :locator, CURRENT_TIMESTAMP)"));
        search.bindValue(QStringLiteral(":source"), id);
        search.bindValue(QStringLiteral(":document"), documentId);
        search.bindValue(QStringLiteral(":text"), text);
        search.bindValue(QStringLiteral(":note"), kind);
        search.bindValue(QStringLiteral(":page"), std::max(page, 0));
        search.bindValue(QStringLiteral(":locator"), locator);
        success = search.exec();
    }
    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    return true;
}

bool LibraryStore::removeAnnotation(const QString &annotationId)
{
    if (!isReady() || annotationId.trimmed().isEmpty()) {
        return false;
    }
    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        return false;
    }
    QSqlQuery search(connection);
    search.prepare(QStringLiteral(
        "DELETE FROM knowledge_items_search WHERE source_id = :id AND source_kind = 'annotation'"));
    search.bindValue(QStringLiteral(":id"), annotationId);
    bool success = search.exec();
    if (success) {
        QSqlQuery query(connection);
        query.prepare(QStringLiteral("DELETE FROM annotations WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), annotationId);
        success = query.exec() && query.numRowsAffected() > 0;
    }
    if (!success || !connection.commit()) {
        connection.rollback();
        return false;
    }
    return true;
}

QVariantList LibraryStore::bookmarksForDocument(const QString &documentId) const
{
    QVariantList results;
    if (!isReady() || documentId.trimmed().isEmpty()) {
        return results;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "SELECT id, label, locator, page, created_at FROM bookmarks "
        "WHERE document_id = :id ORDER BY page, created_at"));
    query.bindValue(QStringLiteral(":id"), documentId);
    if (!query.exec()) {
        return results;
    }
    while (query.next()) {
        results.append(QVariantMap{
            {QStringLiteral("id"), query.value(0)},
            {QStringLiteral("label"), query.value(1)},
            {QStringLiteral("locator"), query.value(2)},
            {QStringLiteral("page"), query.value(3)},
            {QStringLiteral("createdAt"), query.value(4)},
        });
    }
    return results;
}

bool LibraryStore::addBookmark(const QString &documentId,
                               const QString &label,
                               const QString &locator,
                               int page)
{
    if (!isReady() || documentDetails(documentId).isEmpty() || locator.trimmed().isEmpty()) {
        return false;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "INSERT INTO bookmarks (id, document_id, label, locator, page) "
        "VALUES (:id, :document, :label, :locator, :page)"));
    query.bindValue(QStringLiteral(":id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":document"), documentId);
    query.bindValue(QStringLiteral(":label"), label);
    query.bindValue(QStringLiteral(":locator"), locator);
    query.bindValue(QStringLiteral(":page"), std::max(page, 0));
    return query.exec();
}

bool LibraryStore::removeBookmark(const QString &bookmarkId)
{
    if (!isReady() || bookmarkId.trimmed().isEmpty()) {
        return false;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral("DELETE FROM bookmarks WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), bookmarkId);
    return query.exec() && query.numRowsAffected() > 0;
}

QStringList LibraryStore::supportedExtensions() const
{
    QStringList extensions;
    for (const QString &format : supportedFormats()) {
        extensions.append(QStringLiteral("*.") + format);
    }
    return extensions;
}

} // namespace Lumina::Infrastructure

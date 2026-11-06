#include "application/BackupController.h"

#include "infrastructure/database/Database.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#include <QUuid>

#include <algorithm>

namespace {

bool isValidStatus(const QString &status)
{
    return status == QStringLiteral("to_read") || status == QStringLiteral("reading")
        || status == QStringLiteral("read") || status == QStringLiteral("abandoned");
}

bool isValidContentType(const QString &contentType)
{
    static const QStringList types = {
        QStringLiteral("book"), QStringLiteral("manga"), QStringLiteral("manhwa"),
        QStringLiteral("webtoon"), QStringLiteral("newspaper"), QStringLiteral("magazine"),
        QStringLiteral("article"), QStringLiteral("academic"), QStringLiteral("handout"),
        QStringLiteral("comic"), QStringLiteral("document"), QStringLiteral("image"),
        QStringLiteral("text"), QStringLiteral("other"),
    };
    return types.contains(contentType);
}

bool isKnownSetting(const QString &key)
{
    static const QStringList keys = {
        QStringLiteral("theme"), QStringLiteral("font_size"),
        QStringLiteral("line_spacing"), QStringLiteral("continuous_reading"),
        QStringLiteral("high_contrast"), QStringLiteral("speech_rate"),
        QStringLiteral("speech_volume"), QStringLiteral("font_family"),
        QStringLiteral("reader_width"), QStringLiteral("reader_margin"),
        QStringLiteral("text_alignment"), QStringLiteral("image_fit"),
        QStringLiteral("reading_direction"), QStringLiteral("custom_reader_background"),
        QStringLiteral("custom_reader_text"), QStringLiteral("custom_highlight_color"),
    };
    if (keys.contains(key)) {
        return true;
    }

    const QStringList parts = key.split(QChar('.'), Qt::SkipEmptyParts);
    if (parts.size() != 4 || parts.at(0) != QStringLiteral("reader")
        || (parts.at(1) != QStringLiteral("format") && parts.at(1) != QStringLiteral("type")
            && parts.at(1) != QStringLiteral("document"))) {
        return false;
    }
    static const QRegularExpression contextToken(QStringLiteral("^[a-z0-9_-]{1,128}$"));
    if (!contextToken.match(parts.at(2)).hasMatch()) {
        return false;
    }
    static const QStringList readerKeys = {
        QStringLiteral("theme"), QStringLiteral("font_size"), QStringLiteral("line_spacing"),
        QStringLiteral("continuous_reading"), QStringLiteral("high_contrast"),
        QStringLiteral("font_family"), QStringLiteral("reader_width"),
        QStringLiteral("reader_margin"), QStringLiteral("text_alignment"),
        QStringLiteral("image_fit"), QStringLiteral("reading_direction"),
        QStringLiteral("custom_reader_background"), QStringLiteral("custom_reader_text"),
        QStringLiteral("custom_highlight_color"),
    };
    return readerKeys.contains(parts.at(3));
}

bool isValidTheme(const QString &theme)
{
    static const QStringList themes = {
        QStringLiteral("light"), QStringLiteral("dark"), QStringLiteral("sepia"),
        QStringLiteral("paper"), QStringLiteral("gray"), QStringLiteral("high-contrast"),
        QStringLiteral("low-light"),
    };
    return themes.contains(theme);
}

bool isValidColor(const QString &color)
{
    static const QRegularExpression pattern(QStringLiteral("^#[0-9a-fA-F]{6,8}$"));
    return color.isEmpty() || pattern.match(color).hasMatch();
}

} // namespace

namespace Lumina::Application {

BackupController::BackupController(QObject *parent)
    : QObject(parent)
{
}

QString BackupController::lastError() const
{
    return lastError_;
}

void BackupController::setDatabase(Lumina::Infrastructure::Database *database)
{
    database_ = database;
}

void BackupController::fail(const QString &message)
{
    lastError_ = message;
    emit stateChanged();
}

bool BackupController::validatePath(const QString &path)
{
    lastError_.clear();
    if (database_ == nullptr || !database_->isOpen()) {
        fail(QStringLiteral("The local library is not ready yet."));
        return false;
    }
    if (path.trimmed().isEmpty()) {
        fail(QStringLiteral("Choose a destination file first."));
        return false;
    }
    const QFileInfo info(path);
    if (!info.absoluteDir().exists()) {
        fail(QStringLiteral("The destination folder does not exist."));
        return false;
    }
    return true;
}

bool BackupController::validateSourcePath(const QString &path)
{
    lastError_.clear();
    if (database_ == nullptr || !database_->isOpen()) {
        fail(QStringLiteral("The local library is not ready yet."));
        return false;
    }
    const QFileInfo info(path);
    if (path.trimmed().isEmpty() || !info.exists() || !info.isFile() || !info.isReadable()) {
        fail(QStringLiteral("Choose a readable Lumina library export first."));
        return false;
    }
    return true;
}

bool BackupController::validateDatabaseFile(const QString &path, QString *errorMessage) const
{
    const QString connectionName = QStringLiteral("lumina-restore-check-")
        + QUuid::createUuid().toString(QUuid::WithoutBraces);
    bool valid = false;
    QString message;
    {
        QSqlDatabase check = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        check.setDatabaseName(path);
        if (!check.open()) {
            message = QStringLiteral("The selected database backup could not be opened.");
        } else {
            QSqlQuery integrity(check);
            if (!integrity.exec(QStringLiteral("PRAGMA integrity_check")) || !integrity.next()
                || integrity.value(0).toString().compare(QStringLiteral("ok"), Qt::CaseInsensitive) != 0) {
                message = QStringLiteral("The selected database backup failed its integrity check.");
            } else {
                QSqlQuery schema(check);
                if (!schema.exec(QStringLiteral(
                        "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'schema_migrations'") )
                    || !schema.next()) {
                    message = QStringLiteral("The selected file is not a Lumina database backup.");
                } else {
                    valid = true;
                }
            }
        }
        check.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
    return valid;
}

bool BackupController::writeFile(const QString &path, const QByteArray &data)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        fail(QStringLiteral("The export file could not be written."));
        return false;
    }
    lastError_.clear();
    emit stateChanged();
    return true;
}

bool BackupController::exportKnowledge(const QString &path, const QString &format)
{
    if (!validatePath(path)) {
        return false;
    }

    const QString normalizedFormat = format.trimmed().toLower();
    if (normalizedFormat != QStringLiteral("markdown") && normalizedFormat != QStringLiteral("md")
        && normalizedFormat != QStringLiteral("json") && normalizedFormat != QStringLiteral("txt")) {
        fail(QStringLiteral("Choose Markdown, JSON, or plain text for this export."));
        return false;
    }

    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "SELECT 'highlight', h.id, d.title, d.author, h.text, h.note, h.color, h.page, "
        "h.locator, h.created_at FROM highlights h JOIN documents d ON d.id = h.document_id "
        "UNION ALL "
        "SELECT 'annotation', a.id, d.title, d.author, a.text, a.kind, '', a.page, "
        "a.locator, a.created_at FROM annotations a JOIN documents d ON d.id = a.document_id "
        "ORDER BY 10 DESC"));
    if (!query.exec()) {
        fail(QStringLiteral("The knowledge center could not be read for export."));
        return false;
    }

    if (normalizedFormat == QStringLiteral("json")) {
        QJsonArray items;
        while (query.next()) {
            items.append(QJsonObject{
                {QStringLiteral("kind"), query.value(0).toString()},
                {QStringLiteral("id"), query.value(1).toString()},
                {QStringLiteral("documentTitle"), query.value(2).toString()},
                {QStringLiteral("author"), query.value(3).toString()},
                {QStringLiteral("text"), query.value(4).toString()},
                {QStringLiteral("note"), query.value(5).toString()},
                {QStringLiteral("color"), query.value(6).toString()},
                {QStringLiteral("page"), query.value(7).toInt()},
                {QStringLiteral("locator"), query.value(8).toString()},
                {QStringLiteral("createdAt"), query.value(9).toString()},
            });
        }
        const QJsonObject root{
            {QStringLiteral("format"), QStringLiteral("lumina-knowledge")},
            {QStringLiteral("version"), 1},
            {QStringLiteral("exportedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
            {QStringLiteral("items"), items},
        };
        return writeFile(path, QJsonDocument(root).toJson(QJsonDocument::Indented));
    }

    QByteArray output;
    QTextStream stream(&output, QIODevice::WriteOnly);
    if (normalizedFormat == QStringLiteral("markdown") || normalizedFormat == QStringLiteral("md")) {
        stream << "# Lumina Reader knowledge export\n\n";
    } else {
        stream << "Lumina Reader knowledge export\n\n";
    }
    while (query.next()) {
        const QString kind = query.value(0).toString();
        const QString title = query.value(2).toString();
        QString text = query.value(4).toString();
        QString note = query.value(5).toString();
        const int page = query.value(7).toInt() + 1;
        if (normalizedFormat == QStringLiteral("markdown") || normalizedFormat == QStringLiteral("md")) {
            stream << "## " << title << " — page " << page << "\n\n";
            stream << "> " << text.replace('\n', ' ') << "\n\n";
            if (!note.isEmpty()) {
                stream << "**" << (kind == QStringLiteral("annotation") ? "Note" : "Comment")
                       << ":** " << note.replace('\n', ' ') << "\n\n";
            }
        } else {
            stream << title << " [page " << page << "]\n";
            stream << text.replace('\n', ' ') << "\n";
            if (!note.isEmpty()) {
                stream << "Note: " << note.replace('\n', ' ') << "\n";
            }
            stream << "\n";
        }
    }
    stream.flush();
    return writeFile(path, output);
}

bool BackupController::exportLibrary(const QString &path)
{
    if (!validatePath(path)) {
        return false;
    }

    QSqlQuery query(database_->connection());
    if (!query.exec(QStringLiteral(
            "SELECT id, title, author, file_path, file_hash, file_size, format, content_type, "
            "status, progress, current_page, page_count, added_at, COALESCE(last_opened_at, ''), "
            "description, language, is_favorite, rating, page_rotation, reading_seconds, genre, "
            "cover_path "
            "FROM documents ORDER BY title COLLATE NOCASE"))) {
        fail(QStringLiteral("The library could not be read for export."));
        return false;
    }

    QJsonArray documents;
    while (query.next()) {
        QJsonObject documentObject{
            {QStringLiteral("id"), query.value(0).toString()},
            {QStringLiteral("title"), query.value(1).toString()},
            {QStringLiteral("author"), query.value(2).toString()},
            {QStringLiteral("filePath"), query.value(3).toString()},
            {QStringLiteral("fileHash"), query.value(4).toString()},
            {QStringLiteral("fileSize"), query.value(5).toLongLong()},
            {QStringLiteral("format"), query.value(6).toString()},
            {QStringLiteral("contentType"), query.value(7).toString()},
            {QStringLiteral("status"), query.value(8).toString()},
            {QStringLiteral("progress"), query.value(9).toDouble()},
            {QStringLiteral("currentPage"), query.value(10).toInt()},
            {QStringLiteral("pageCount"), query.value(11).toInt()},
            {QStringLiteral("addedAt"), query.value(12).toString()},
            {QStringLiteral("lastOpenedAt"), query.value(13).toString()},
            {QStringLiteral("description"), query.value(14).toString()},
            {QStringLiteral("language"), query.value(15).toString()},
            {QStringLiteral("isFavorite"), query.value(16).toInt() != 0},
            {QStringLiteral("rating"), query.value(17).toInt()},
            {QStringLiteral("pageRotation"), query.value(18).toInt()},
            {QStringLiteral("readingSeconds"), query.value(19).toLongLong()},
            {QStringLiteral("genre"), query.value(20).toString()},
            {QStringLiteral("coverPath"), query.value(21).toString()},
        };

        const QString documentId = query.value(0).toString();
        QJsonArray collections;
        QSqlQuery collectionQuery(database_->connection());
        collectionQuery.prepare(QStringLiteral(
            "SELECT c.name, c.description FROM collections c "
            "JOIN document_collections dc ON dc.collection_id = c.id "
            "WHERE dc.document_id = :document ORDER BY c.name COLLATE NOCASE"));
        collectionQuery.bindValue(QStringLiteral(":document"), documentId);
        if (!collectionQuery.exec()) {
            fail(QStringLiteral("The library collections could not be read for export."));
            return false;
        }
        while (collectionQuery.next()) {
            collections.append(QJsonObject{
                {QStringLiteral("name"), collectionQuery.value(0).toString()},
                {QStringLiteral("description"), collectionQuery.value(1).toString()},
            });
        }
        documentObject.insert(QStringLiteral("collections"), collections);

        QJsonArray tags;
        QSqlQuery tagQuery(database_->connection());
        tagQuery.prepare(QStringLiteral(
            "SELECT t.name FROM tags t JOIN document_tags dt ON dt.tag_id = t.id "
            "WHERE dt.document_id = :document ORDER BY t.name COLLATE NOCASE"));
        tagQuery.bindValue(QStringLiteral(":document"), documentId);
        if (!tagQuery.exec()) {
            fail(QStringLiteral("The library tags could not be read for export."));
            return false;
        }
        while (tagQuery.next()) {
            tags.append(tagQuery.value(0).toString());
        }
        documentObject.insert(QStringLiteral("tags"), tags);

        QJsonArray highlights;
        QSqlQuery highlightQuery(database_->connection());
        highlightQuery.prepare(QStringLiteral(
            "SELECT id, text, note, color, style, locator, context_before, context_after, page, "
            "created_at, updated_at FROM highlights WHERE document_id = :document "
            "ORDER BY page, created_at"));
        highlightQuery.bindValue(QStringLiteral(":document"), documentId);
        if (!highlightQuery.exec()) {
            fail(QStringLiteral("The library highlights could not be read for export."));
            return false;
        }
        while (highlightQuery.next()) {
            highlights.append(QJsonObject{
                {QStringLiteral("id"), highlightQuery.value(0).toString()},
                {QStringLiteral("text"), highlightQuery.value(1).toString()},
                {QStringLiteral("note"), highlightQuery.value(2).toString()},
                {QStringLiteral("color"), highlightQuery.value(3).toString()},
                {QStringLiteral("style"), highlightQuery.value(4).toString()},
                {QStringLiteral("locator"), highlightQuery.value(5).toString()},
                {QStringLiteral("contextBefore"), highlightQuery.value(6).toString()},
                {QStringLiteral("contextAfter"), highlightQuery.value(7).toString()},
                {QStringLiteral("page"), highlightQuery.value(8).toInt()},
                {QStringLiteral("createdAt"), highlightQuery.value(9).toString()},
                {QStringLiteral("updatedAt"), highlightQuery.value(10).toString()},
            });
        }
        documentObject.insert(QStringLiteral("highlights"), highlights);

        QJsonArray annotations;
        QSqlQuery annotationQuery(database_->connection());
        annotationQuery.prepare(QStringLiteral(
            "SELECT id, text, kind, locator, page, x, y, width, height, created_at, updated_at "
            "FROM annotations WHERE document_id = :document ORDER BY page, created_at"));
        annotationQuery.bindValue(QStringLiteral(":document"), documentId);
        if (!annotationQuery.exec()) {
            fail(QStringLiteral("The library annotations could not be read for export."));
            return false;
        }
        while (annotationQuery.next()) {
            annotations.append(QJsonObject{
                {QStringLiteral("id"), annotationQuery.value(0).toString()},
                {QStringLiteral("text"), annotationQuery.value(1).toString()},
                {QStringLiteral("kind"), annotationQuery.value(2).toString()},
                {QStringLiteral("locator"), annotationQuery.value(3).toString()},
                {QStringLiteral("page"), annotationQuery.value(4).toInt()},
                {QStringLiteral("x"), annotationQuery.value(5).toDouble()},
                {QStringLiteral("y"), annotationQuery.value(6).toDouble()},
                {QStringLiteral("width"), annotationQuery.value(7).toDouble()},
                {QStringLiteral("height"), annotationQuery.value(8).toDouble()},
                {QStringLiteral("createdAt"), annotationQuery.value(9).toString()},
                {QStringLiteral("updatedAt"), annotationQuery.value(10).toString()},
            });
        }
        documentObject.insert(QStringLiteral("annotations"), annotations);

        QJsonArray bookmarks;
        QSqlQuery bookmarkQuery(database_->connection());
        bookmarkQuery.prepare(QStringLiteral(
            "SELECT id, label, locator, page, created_at FROM bookmarks "
            "WHERE document_id = :document ORDER BY page, created_at"));
        bookmarkQuery.bindValue(QStringLiteral(":document"), documentId);
        if (!bookmarkQuery.exec()) {
            fail(QStringLiteral("The library bookmarks could not be read for export."));
            return false;
        }
        while (bookmarkQuery.next()) {
            bookmarks.append(QJsonObject{
                {QStringLiteral("id"), bookmarkQuery.value(0).toString()},
                {QStringLiteral("label"), bookmarkQuery.value(1).toString()},
                {QStringLiteral("locator"), bookmarkQuery.value(2).toString()},
                {QStringLiteral("page"), bookmarkQuery.value(3).toInt()},
                {QStringLiteral("createdAt"), bookmarkQuery.value(4).toString()},
            });
        }
        documentObject.insert(QStringLiteral("bookmarks"), bookmarks);

        QJsonArray readingHistory;
        QSqlQuery historyQuery(database_->connection());
        historyQuery.prepare(QStringLiteral(
            "SELECT progress, page, recorded_at FROM reading_history "
            "WHERE document_id = :document ORDER BY recorded_at"));
        historyQuery.bindValue(QStringLiteral(":document"), documentId);
        if (!historyQuery.exec()) {
            fail(QStringLiteral("The reading history could not be read for export."));
            return false;
        }
        while (historyQuery.next()) {
            readingHistory.append(QJsonObject{
                {QStringLiteral("progress"), historyQuery.value(0).toDouble()},
                {QStringLiteral("page"), historyQuery.value(1).toInt()},
                {QStringLiteral("recordedAt"), historyQuery.value(2).toString()},
            });
        }
        documentObject.insert(QStringLiteral("readingHistory"), readingHistory);

        QJsonArray ocrPages;
        QSqlQuery ocrQuery(database_->connection());
        ocrQuery.prepare(QStringLiteral(
            "SELECT page, language, text, status, updated_at FROM ocr_pages "
            "WHERE document_id = :document ORDER BY page"));
        ocrQuery.bindValue(QStringLiteral(":document"), documentId);
        if (!ocrQuery.exec()) {
            fail(QStringLiteral("The OCR data could not be read for export."));
            return false;
        }
        while (ocrQuery.next()) {
            ocrPages.append(QJsonObject{
                {QStringLiteral("page"), ocrQuery.value(0).toInt()},
                {QStringLiteral("language"), ocrQuery.value(1).toString()},
                {QStringLiteral("text"), ocrQuery.value(2).toString()},
                {QStringLiteral("status"), ocrQuery.value(3).toString()},
                {QStringLiteral("updatedAt"), ocrQuery.value(4).toString()},
            });
        }
        documentObject.insert(QStringLiteral("ocrPages"), ocrPages);
        documents.append(documentObject);
    }

    const QJsonObject root{
        {QStringLiteral("format"), QStringLiteral("lumina-library")},
        {QStringLiteral("version"), 2},
        {QStringLiteral("exportedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {QStringLiteral("documents"), documents},
    };
    return writeFile(path, QJsonDocument(root).toJson(QJsonDocument::Indented));
}

bool BackupController::exportSettings(const QString &path)
{
    if (!validatePath(path)) {
        return false;
    }

    QSqlQuery query(database_->connection());
    if (!query.exec(QStringLiteral("SELECT key, value FROM app_settings ORDER BY key"))) {
        fail(QStringLiteral("The reading preferences could not be read for export."));
        return false;
    }

    QJsonObject settings;
    while (query.next()) {
        const QString key = query.value(0).toString();
        if (!isKnownSetting(key)) {
            continue;
        }
        const QString validationKey = key.startsWith(QStringLiteral("reader."))
            ? key.section(QChar('.'), -1)
            : key;
        const QString value = query.value(1).toString();
        if (validationKey == QStringLiteral("continuous_reading")
            || validationKey == QStringLiteral("high_contrast")) {
            settings.insert(key, value == QStringLiteral("1"));
        } else if (validationKey == QStringLiteral("font_size")
                   || validationKey == QStringLiteral("line_spacing")
                   || validationKey == QStringLiteral("speech_rate")
                   || validationKey == QStringLiteral("speech_volume")
                   || validationKey == QStringLiteral("reader_width")
                   || validationKey == QStringLiteral("reader_margin")) {
            settings.insert(key, value.toInt());
        } else {
            settings.insert(key, value);
        }
    }

    const QJsonObject root{
        {QStringLiteral("format"), QStringLiteral("lumina-settings")},
        {QStringLiteral("version"), 1},
        {QStringLiteral("exportedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {QStringLiteral("settings"), settings},
    };
    return writeFile(path, QJsonDocument(root).toJson(QJsonDocument::Indented));
}

bool BackupController::restoreLibrary(const QString &path)
{
    if (!validateSourcePath(path)) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 128 * 1024 * 1024) {
        fail(QStringLiteral("The Lumina library export could not be opened."));
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        fail(QStringLiteral("The selected file is not a valid Lumina library export."));
        return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("lumina-library")
        || !root.value(QStringLiteral("documents")).isArray()
        || (root.value(QStringLiteral("version")).isDouble()
            && (root.value(QStringLiteral("version")).toInt() < 1
                || root.value(QStringLiteral("version")).toInt() > 2))) {
        fail(QStringLiteral("The selected file is not a Lumina library export."));
        return false;
    }

    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        fail(QStringLiteral("The library restore could not be started."));
        return false;
    }

    int restored = 0;
    const auto uuid = [] {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    };
    const auto existingChildId = [&](const QString &table,
                                     const QString &requestedId,
                                     const QString &documentId) {
        const QString candidate = requestedId.trimmed();
        if (candidate.isEmpty()) {
            return uuid();
        }
        QSqlQuery check(connection);
        check.prepare(QStringLiteral("SELECT document_id FROM %1 WHERE id = :id").arg(table));
        check.bindValue(QStringLiteral(":id"), candidate);
        if (!check.exec() || !check.next() || check.value(0).toString() == documentId) {
            return candidate;
        }
        return uuid();
    };

    const auto collectionId = [&](const QString &name, const QString &description) {
        QSqlQuery find(connection);
        find.prepare(QStringLiteral("SELECT id FROM collections WHERE name = :name"));
        find.bindValue(QStringLiteral(":name"), name);
        if (find.exec() && find.next()) {
            const QString id = find.value(0).toString();
            QSqlQuery update(connection);
            update.prepare(QStringLiteral(
                "UPDATE collections SET description = :description, updated_at = CURRENT_TIMESTAMP "
                "WHERE id = :id"));
            update.bindValue(QStringLiteral(":description"), description);
            update.bindValue(QStringLiteral(":id"), id);
            if (!update.exec()) {
                return QString();
            }
            return id;
        }
        const QString id = uuid();
        QSqlQuery insert(connection);
        insert.prepare(QStringLiteral(
            "INSERT INTO collections (id, name, description) VALUES (:id, :name, :description)"));
        insert.bindValue(QStringLiteral(":id"), id);
        insert.bindValue(QStringLiteral(":name"), name);
        insert.bindValue(QStringLiteral(":description"), description);
        return insert.exec() ? id : QString();
    };

    const auto tagId = [&](const QString &name) {
        QSqlQuery find(connection);
        find.prepare(QStringLiteral("SELECT id FROM tags WHERE name = :name"));
        find.bindValue(QStringLiteral(":name"), name);
        if (find.exec() && find.next()) {
            return find.value(0).toString();
        }
        const QString id = uuid();
        QSqlQuery insert(connection);
        insert.prepare(QStringLiteral("INSERT INTO tags (id, name) VALUES (:id, :name)"));
        insert.bindValue(QStringLiteral(":id"), id);
        insert.bindValue(QStringLiteral(":name"), name);
        return insert.exec() ? id : QString();
    };

    for (const QJsonValue &value : root.value(QStringLiteral("documents")).toArray()) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject item = value.toObject();
        const QString hash = item.value(QStringLiteral("fileHash")).toString().trimmed();
        const QString filePath = item.value(QStringLiteral("filePath")).toString().trimmed();
        if (hash.isEmpty() && filePath.isEmpty()) {
            continue;
        }

        QSqlQuery find(connection);
        find.prepare(QStringLiteral(
            "SELECT id FROM documents WHERE "
            "(:hash <> '' AND file_hash = :hash) OR "
            "(:path <> '' AND (file_path = :path OR canonical_path = :path)) LIMIT 1"));
        find.bindValue(QStringLiteral(":hash"), hash);
        find.bindValue(QStringLiteral(":path"), filePath);
        if (!find.exec() || !find.next()) {
            continue;
        }

        const QString status = item.value(QStringLiteral("status")).toString();
        const QString restoredStatus = isValidStatus(status) ? status : QStringLiteral("to_read");
        const double progress = std::clamp(item.value(QStringLiteral("progress")).toDouble(), 0.0, 1.0);
        const int page = std::max(0, item.value(QStringLiteral("currentPage")).toInt());
        const int pageCount = std::max(0, item.value(QStringLiteral("pageCount")).toInt());
        const int rating = std::clamp(item.value(QStringLiteral("rating")).toInt(), 0, 5);
        const int requestedRotation = item.value(QStringLiteral("pageRotation")).toInt();
        const int rotation = requestedRotation == 90 || requestedRotation == 180
                || requestedRotation == 270
            ? requestedRotation
            : 0;
        const qint64 readingSeconds = std::max<qint64>(
            0, item.value(QStringLiteral("readingSeconds")).toVariant().toLongLong());
        const QString requestedContentType = item.value(QStringLiteral("contentType")).toString().trimmed().toLower();
        const QString contentType = isValidContentType(requestedContentType)
            ? requestedContentType
            : QStringLiteral("other");
        const QString genre = item.value(QStringLiteral("genre")).toString().trimmed().left(120);

        const QString localDocumentId = find.value(0).toString();
        QString coverPath;
        const QFileInfo coverInfo(item.value(QStringLiteral("coverPath")).toString());
        if (coverInfo.exists() && coverInfo.isFile() && coverInfo.isReadable()) {
            coverPath = coverInfo.absoluteFilePath();
        }

        QSqlQuery update(connection);
        update.prepare(QStringLiteral(
            "UPDATE documents SET title = :title, author = :author, description = :description, "
            "language = :language, content_type = :content_type, genre = :genre, "
            "status = :status, progress = :progress, current_page = :page, "
            "page_count = :page_count, last_opened_at = NULLIF(:last_opened, ''), "
            "is_favorite = :favorite, rating = :rating, page_rotation = :rotation, "
            "reading_seconds = :reading_seconds, "
            "cover_path = CASE WHEN :cover_path <> '' THEN :cover_path ELSE cover_path END, "
            "updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
        update.bindValue(QStringLiteral(":title"), item.value(QStringLiteral("title")).toString());
        update.bindValue(QStringLiteral(":author"), item.value(QStringLiteral("author")).toString());
        update.bindValue(QStringLiteral(":description"), item.value(QStringLiteral("description")).toString());
        update.bindValue(QStringLiteral(":language"), item.value(QStringLiteral("language")).toString());
        update.bindValue(QStringLiteral(":content_type"), contentType);
        update.bindValue(QStringLiteral(":genre"), genre);
        update.bindValue(QStringLiteral(":status"), restoredStatus);
        update.bindValue(QStringLiteral(":progress"), progress);
        update.bindValue(QStringLiteral(":page"), page);
        update.bindValue(QStringLiteral(":page_count"), pageCount);
        update.bindValue(QStringLiteral(":last_opened"), item.value(QStringLiteral("lastOpenedAt")).toString());
        update.bindValue(QStringLiteral(":favorite"), item.value(QStringLiteral("isFavorite")).toBool() ? 1 : 0);
        update.bindValue(QStringLiteral(":rating"), rating);
        update.bindValue(QStringLiteral(":rotation"), rotation);
        update.bindValue(QStringLiteral(":reading_seconds"), readingSeconds);
        update.bindValue(QStringLiteral(":cover_path"), coverPath);
        update.bindValue(QStringLiteral(":id"), localDocumentId);
        if (!update.exec()) {
            connection.rollback();
            fail(QStringLiteral("The library restore could not be completed."));
            return false;
        }

        QSqlQuery search(connection);
        search.prepare(QStringLiteral(
            "UPDATE document_search SET title = :title, author = :author, description = :description "
            "WHERE document_id = :id"));
        search.bindValue(QStringLiteral(":title"), item.value(QStringLiteral("title")).toString());
        search.bindValue(QStringLiteral(":author"), item.value(QStringLiteral("author")).toString());
        search.bindValue(QStringLiteral(":description"), item.value(QStringLiteral("description")).toString());
        search.bindValue(QStringLiteral(":id"), localDocumentId);
        if (!search.exec()) {
            connection.rollback();
            fail(QStringLiteral("The library search index could not be restored."));
            return false;
        }

        for (const QJsonValue &collectionValue : item.value(QStringLiteral("collections")).toArray()) {
            const QJsonObject collection = collectionValue.toObject();
            const QString name = collection.value(QStringLiteral("name")).toString().trimmed();
            if (name.isEmpty() || name.size() > 200) {
                continue;
            }
            const QString id = collectionId(name,
                                           collection.value(QStringLiteral("description")).toString());
            if (id.isEmpty()) {
                connection.rollback();
                fail(QStringLiteral("The library collections could not be restored."));
                return false;
            }
            QSqlQuery relation(connection);
            relation.prepare(QStringLiteral(
                "INSERT OR IGNORE INTO document_collections (document_id, collection_id) "
                "VALUES (:document, :collection)"));
            relation.bindValue(QStringLiteral(":document"), localDocumentId);
            relation.bindValue(QStringLiteral(":collection"), id);
            if (!relation.exec()) {
                connection.rollback();
                fail(QStringLiteral("The library collections could not be restored."));
                return false;
            }
        }

        for (const QJsonValue &tagValue : item.value(QStringLiteral("tags")).toArray()) {
            const QString name = tagValue.isString() ? tagValue.toString().trimmed() : QString();
            if (name.isEmpty() || name.size() > 120) {
                continue;
            }
            const QString id = tagId(name);
            if (id.isEmpty()) {
                connection.rollback();
                fail(QStringLiteral("The library tags could not be restored."));
                return false;
            }
            QSqlQuery relation(connection);
            relation.prepare(QStringLiteral(
                "INSERT OR IGNORE INTO document_tags (document_id, tag_id) VALUES (:document, :tag)"));
            relation.bindValue(QStringLiteral(":document"), localDocumentId);
            relation.bindValue(QStringLiteral(":tag"), id);
            if (!relation.exec()) {
                connection.rollback();
                fail(QStringLiteral("The library tags could not be restored."));
                return false;
            }
        }

        for (const QJsonValue &highlightValue : item.value(QStringLiteral("highlights")).toArray()) {
            const QJsonObject highlight = highlightValue.toObject();
            const QString text = highlight.value(QStringLiteral("text")).toString();
            const QString locator = highlight.value(QStringLiteral("locator")).toString().trimmed();
            if (text.trimmed().isEmpty() || locator.isEmpty()) {
                continue;
            }
            const QString id = existingChildId(QStringLiteral("highlights"),
                                               highlight.value(QStringLiteral("id")).toString(),
                                               localDocumentId);
            const QString style = highlight.value(QStringLiteral("style")).toString().trimmed().toLower();
            const QString normalizedStyle = style == QStringLiteral("underline")
                    || style == QStringLiteral("strike")
                ? style
                : QStringLiteral("highlight");
            const QString color = highlight.value(QStringLiteral("color")).toString().trimmed().isEmpty()
                ? QStringLiteral("#f6c945")
                : highlight.value(QStringLiteral("color")).toString().trimmed();
            QSqlQuery removeSearch(connection);
            removeSearch.prepare(QStringLiteral(
                "DELETE FROM knowledge_items_search WHERE source_id = :id AND source_kind = 'highlight'"));
            removeSearch.bindValue(QStringLiteral(":id"), id);
            if (!removeSearch.exec()) {
                connection.rollback();
                fail(QStringLiteral("The library highlights could not be restored."));
                return false;
            }
            QSqlQuery insert(connection);
            insert.prepare(QStringLiteral(
                "INSERT INTO highlights "
                "(id, document_id, text, note, color, style, locator, context_before, context_after, "
                "page, created_at, updated_at) VALUES "
                "(:id, :document, :text, :note, :color, :style, :locator, :before, :after, :page, "
                "COALESCE(NULLIF(:created_at, ''), CURRENT_TIMESTAMP), "
                "COALESCE(NULLIF(:updated_at, ''), CURRENT_TIMESTAMP)) "
                "ON CONFLICT(id) DO UPDATE SET document_id = excluded.document_id, text = excluded.text, "
                "note = excluded.note, color = excluded.color, style = excluded.style, locator = excluded.locator, "
                "context_before = excluded.context_before, context_after = excluded.context_after, "
                "page = excluded.page, created_at = excluded.created_at, updated_at = excluded.updated_at"));
            insert.bindValue(QStringLiteral(":id"), id);
            insert.bindValue(QStringLiteral(":document"), localDocumentId);
            insert.bindValue(QStringLiteral(":text"), text.left(4 * 1024 * 1024));
            insert.bindValue(QStringLiteral(":note"), highlight.value(QStringLiteral("note")).toString().left(1024 * 1024));
            insert.bindValue(QStringLiteral(":color"), color.left(32));
            insert.bindValue(QStringLiteral(":style"), normalizedStyle);
            insert.bindValue(QStringLiteral(":locator"), locator.left(4096));
            insert.bindValue(QStringLiteral(":before"), highlight.value(QStringLiteral("contextBefore")).toString().left(4096));
            insert.bindValue(QStringLiteral(":after"), highlight.value(QStringLiteral("contextAfter")).toString().left(4096));
            insert.bindValue(QStringLiteral(":page"), std::max(0, highlight.value(QStringLiteral("page")).toInt()));
            insert.bindValue(QStringLiteral(":created_at"), highlight.value(QStringLiteral("createdAt")).toString());
            insert.bindValue(QStringLiteral(":updated_at"), highlight.value(QStringLiteral("updatedAt")).toString());
            if (!insert.exec()) {
                connection.rollback();
                fail(QStringLiteral("The library highlights could not be restored."));
                return false;
            }
            QSqlQuery search(connection);
            search.prepare(QStringLiteral(
                "INSERT INTO knowledge_items_search "
                "(source_id, source_kind, document_id, text, note, page, locator, created_at) "
                "VALUES (:id, 'highlight', :document, :text, :note, :page, :locator, "
                "COALESCE(NULLIF(:created_at, ''), CURRENT_TIMESTAMP))"));
            search.bindValue(QStringLiteral(":id"), id);
            search.bindValue(QStringLiteral(":document"), localDocumentId);
            search.bindValue(QStringLiteral(":text"), text);
            search.bindValue(QStringLiteral(":note"), highlight.value(QStringLiteral("note")).toString());
            search.bindValue(QStringLiteral(":page"), std::max(0, highlight.value(QStringLiteral("page")).toInt()));
            search.bindValue(QStringLiteral(":locator"), locator);
            search.bindValue(QStringLiteral(":created_at"), highlight.value(QStringLiteral("createdAt")).toString());
            if (!search.exec()) {
                connection.rollback();
                fail(QStringLiteral("The library search index could not be restored."));
                return false;
            }
        }

        for (const QJsonValue &annotationValue : item.value(QStringLiteral("annotations")).toArray()) {
            const QJsonObject annotation = annotationValue.toObject();
            const QString text = annotation.value(QStringLiteral("text")).toString();
            const QString locator = annotation.value(QStringLiteral("locator")).toString().trimmed();
            if (text.trimmed().isEmpty() || locator.isEmpty()) {
                continue;
            }
            const QString id = existingChildId(QStringLiteral("annotations"),
                                               annotation.value(QStringLiteral("id")).toString(),
                                               localDocumentId);
            const QString kind = annotation.value(QStringLiteral("kind")).toString().trimmed().isEmpty()
                ? QStringLiteral("note")
                : annotation.value(QStringLiteral("kind")).toString().trimmed().left(64);
            QSqlQuery removeSearch(connection);
            removeSearch.prepare(QStringLiteral(
                "DELETE FROM knowledge_items_search WHERE source_id = :id AND source_kind = 'annotation'"));
            removeSearch.bindValue(QStringLiteral(":id"), id);
            if (!removeSearch.exec()) {
                connection.rollback();
                fail(QStringLiteral("The library annotations could not be restored."));
                return false;
            }
            QSqlQuery insert(connection);
            insert.prepare(QStringLiteral(
                "INSERT INTO annotations "
                "(id, document_id, text, kind, locator, page, x, y, width, height, created_at, updated_at) "
                "VALUES (:id, :document, :text, :kind, :locator, :page, :x, :y, :width, :height, "
                "COALESCE(NULLIF(:created_at, ''), CURRENT_TIMESTAMP), "
                "COALESCE(NULLIF(:updated_at, ''), CURRENT_TIMESTAMP)) "
                "ON CONFLICT(id) DO UPDATE SET document_id = excluded.document_id, text = excluded.text, "
                "kind = excluded.kind, locator = excluded.locator, page = excluded.page, x = excluded.x, "
                "y = excluded.y, width = excluded.width, height = excluded.height, "
                "created_at = excluded.created_at, updated_at = excluded.updated_at"));
            insert.bindValue(QStringLiteral(":id"), id);
            insert.bindValue(QStringLiteral(":document"), localDocumentId);
            insert.bindValue(QStringLiteral(":text"), text.left(4 * 1024 * 1024));
            insert.bindValue(QStringLiteral(":kind"), kind);
            insert.bindValue(QStringLiteral(":locator"), locator.left(4096));
            insert.bindValue(QStringLiteral(":page"), std::max(0, annotation.value(QStringLiteral("page")).toInt()));
            insert.bindValue(QStringLiteral(":x"), std::clamp(annotation.value(QStringLiteral("x")).toDouble(), 0.0, 1.0));
            insert.bindValue(QStringLiteral(":y"), std::clamp(annotation.value(QStringLiteral("y")).toDouble(), 0.0, 1.0));
            insert.bindValue(QStringLiteral(":width"), std::clamp(annotation.value(QStringLiteral("width")).toDouble(), 0.0, 1.0));
            insert.bindValue(QStringLiteral(":height"), std::clamp(annotation.value(QStringLiteral("height")).toDouble(), 0.0, 1.0));
            insert.bindValue(QStringLiteral(":created_at"), annotation.value(QStringLiteral("createdAt")).toString());
            insert.bindValue(QStringLiteral(":updated_at"), annotation.value(QStringLiteral("updatedAt")).toString());
            if (!insert.exec()) {
                connection.rollback();
                fail(QStringLiteral("The library annotations could not be restored."));
                return false;
            }
            if (kind != QStringLiteral("ocr-word")) {
                QSqlQuery search(connection);
                search.prepare(QStringLiteral(
                    "INSERT INTO knowledge_items_search "
                    "(source_id, source_kind, document_id, text, note, page, locator, created_at) "
                    "VALUES (:id, 'annotation', :document, :text, :note, :page, :locator, "
                    "COALESCE(NULLIF(:created_at, ''), CURRENT_TIMESTAMP))"));
                search.bindValue(QStringLiteral(":id"), id);
                search.bindValue(QStringLiteral(":document"), localDocumentId);
                search.bindValue(QStringLiteral(":text"), text);
                search.bindValue(QStringLiteral(":note"), kind);
                search.bindValue(QStringLiteral(":page"), std::max(0, annotation.value(QStringLiteral("page")).toInt()));
                search.bindValue(QStringLiteral(":locator"), locator);
                search.bindValue(QStringLiteral(":created_at"), annotation.value(QStringLiteral("createdAt")).toString());
                if (!search.exec()) {
                    connection.rollback();
                    fail(QStringLiteral("The library search index could not be restored."));
                    return false;
                }
            }
        }

        for (const QJsonValue &bookmarkValue : item.value(QStringLiteral("bookmarks")).toArray()) {
            const QJsonObject bookmark = bookmarkValue.toObject();
            const QString locator = bookmark.value(QStringLiteral("locator")).toString().trimmed();
            if (locator.isEmpty()) {
                continue;
            }
            const QString id = existingChildId(QStringLiteral("bookmarks"),
                                               bookmark.value(QStringLiteral("id")).toString(),
                                               localDocumentId);
            QSqlQuery insert(connection);
            insert.prepare(QStringLiteral(
                "INSERT INTO bookmarks (id, document_id, label, locator, page, created_at) "
                "VALUES (:id, :document, :label, :locator, :page, "
                "COALESCE(NULLIF(:created_at, ''), CURRENT_TIMESTAMP)) "
                "ON CONFLICT(id) DO UPDATE SET document_id = excluded.document_id, label = excluded.label, "
                "locator = excluded.locator, page = excluded.page, created_at = excluded.created_at"));
            insert.bindValue(QStringLiteral(":id"), id);
            insert.bindValue(QStringLiteral(":document"), localDocumentId);
            insert.bindValue(QStringLiteral(":label"), bookmark.value(QStringLiteral("label")).toString().left(512));
            insert.bindValue(QStringLiteral(":locator"), locator.left(4096));
            insert.bindValue(QStringLiteral(":page"), std::max(0, bookmark.value(QStringLiteral("page")).toInt()));
            insert.bindValue(QStringLiteral(":created_at"), bookmark.value(QStringLiteral("createdAt")).toString());
            if (!insert.exec()) {
                connection.rollback();
                fail(QStringLiteral("The library bookmarks could not be restored."));
                return false;
            }
        }

        for (const QJsonValue &historyValue : item.value(QStringLiteral("readingHistory")).toArray()) {
            const QJsonObject history = historyValue.toObject();
            const double progress = std::clamp(history.value(QStringLiteral("progress")).toDouble(), 0.0, 1.0);
            const int page = std::max(0, history.value(QStringLiteral("page")).toInt());
            const QString recordedAt = history.value(QStringLiteral("recordedAt")).toString();
            QSqlQuery exists(connection);
            exists.prepare(QStringLiteral(
                "SELECT 1 FROM reading_history WHERE document_id = :document AND progress = :progress "
                "AND page = :page AND recorded_at = :recorded LIMIT 1"));
            exists.bindValue(QStringLiteral(":document"), localDocumentId);
            exists.bindValue(QStringLiteral(":progress"), progress);
            exists.bindValue(QStringLiteral(":page"), page);
            exists.bindValue(QStringLiteral(":recorded"), recordedAt);
            if (!exists.exec()) {
                connection.rollback();
                fail(QStringLiteral("The reading history could not be restored."));
                return false;
            }
            if (exists.next()) {
                continue;
            }
            QSqlQuery insert(connection);
            insert.prepare(QStringLiteral(
                "INSERT INTO reading_history (document_id, progress, page, recorded_at) "
                "VALUES (:document, :progress, :page, COALESCE(NULLIF(:recorded, ''), CURRENT_TIMESTAMP))"));
            insert.bindValue(QStringLiteral(":document"), localDocumentId);
            insert.bindValue(QStringLiteral(":progress"), progress);
            insert.bindValue(QStringLiteral(":page"), page);
            insert.bindValue(QStringLiteral(":recorded"), recordedAt);
            if (!insert.exec()) {
                connection.rollback();
                fail(QStringLiteral("The reading history could not be restored."));
                return false;
            }
        }

        for (const QJsonValue &ocrValue : item.value(QStringLiteral("ocrPages")).toArray()) {
            const QJsonObject ocr = ocrValue.toObject();
            const int page = std::max(0, ocr.value(QStringLiteral("page")).toInt());
            const QString status = ocr.value(QStringLiteral("status")).toString().trimmed().toLower();
            if (status != QStringLiteral("pending") && status != QStringLiteral("completed")
                && status != QStringLiteral("failed")) {
                continue;
            }
            const QString sourceId = localDocumentId + QStringLiteral(":") + QString::number(page);
            QSqlQuery removeSearch(connection);
            removeSearch.prepare(QStringLiteral(
                "DELETE FROM knowledge_items_search WHERE source_id = :source AND source_kind = 'ocr'"));
            removeSearch.bindValue(QStringLiteral(":source"), sourceId);
            if (!removeSearch.exec()) {
                connection.rollback();
                fail(QStringLiteral("The OCR data could not be restored."));
                return false;
            }
            QSqlQuery insert(connection);
            insert.prepare(QStringLiteral(
                "INSERT INTO ocr_pages (document_id, page, language, text, status, updated_at) "
                "VALUES (:document, :page, :language, :text, :status, "
                "COALESCE(NULLIF(:updated_at, ''), CURRENT_TIMESTAMP)) "
                "ON CONFLICT(document_id, page) DO UPDATE SET language = excluded.language, "
                "text = excluded.text, status = excluded.status, updated_at = excluded.updated_at"));
            insert.bindValue(QStringLiteral(":document"), localDocumentId);
            insert.bindValue(QStringLiteral(":page"), page);
            insert.bindValue(QStringLiteral(":language"), ocr.value(QStringLiteral("language")).toString().left(32));
            insert.bindValue(QStringLiteral(":text"), ocr.value(QStringLiteral("text")).toString().left(4 * 1024 * 1024));
            insert.bindValue(QStringLiteral(":status"), status);
            insert.bindValue(QStringLiteral(":updated_at"), ocr.value(QStringLiteral("updatedAt")).toString());
            if (!insert.exec()) {
                connection.rollback();
                fail(QStringLiteral("The OCR data could not be restored."));
                return false;
            }
            const QString text = ocr.value(QStringLiteral("text")).toString().left(4 * 1024 * 1024);
            if (status == QStringLiteral("completed") && !text.trimmed().isEmpty()) {
                QSqlQuery search(connection);
                search.prepare(QStringLiteral(
                    "INSERT INTO knowledge_items_search "
                    "(source_id, source_kind, document_id, text, note, page, locator, created_at) "
                    "VALUES (:source, 'ocr', :document, :text, '', :page, :locator, "
                    "COALESCE(NULLIF(:created_at, ''), CURRENT_TIMESTAMP))"));
                search.bindValue(QStringLiteral(":source"), sourceId);
                search.bindValue(QStringLiteral(":document"), localDocumentId);
                search.bindValue(QStringLiteral(":text"), text);
                search.bindValue(QStringLiteral(":page"), page);
                search.bindValue(QStringLiteral(":locator"), QStringLiteral("ocr:") + QString::number(page));
                search.bindValue(QStringLiteral(":created_at"), ocr.value(QStringLiteral("updatedAt")).toString());
                if (!search.exec()) {
                    connection.rollback();
                    fail(QStringLiteral("The library search index could not be restored."));
                    return false;
                }
            }
        }
        ++restored;
    }

    if (restored == 0) {
        connection.rollback();
        fail(QStringLiteral("No documents from this export matched the current library."));
        return false;
    }
    if (!connection.commit()) {
        connection.rollback();
        fail(QStringLiteral("The library restore could not be saved."));
        return false;
    }
    lastError_.clear();
    emit stateChanged();
    return true;
}

bool BackupController::restoreSettings(const QString &path)
{
    if (!validateSourcePath(path)) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 4 * 1024 * 1024) {
        fail(QStringLiteral("The selected settings export could not be read."));
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        fail(QStringLiteral("The selected file is not a valid Lumina settings export."));
        return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("lumina-settings")
        || !root.value(QStringLiteral("settings")).isObject()) {
        fail(QStringLiteral("The selected file is not a Lumina settings export."));
        return false;
    }

    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        fail(QStringLiteral("The reading preferences restore could not be started."));
        return false;
    }

    int restored = 0;
    const QJsonObject settings = root.value(QStringLiteral("settings")).toObject();
    for (auto iterator = settings.constBegin(); iterator != settings.constEnd(); ++iterator) {
        const QString key = iterator.key();
        const QJsonValue value = iterator.value();
        if (!isKnownSetting(key)) {
            continue;
        }
        const QString validationKey = key.startsWith(QStringLiteral("reader."))
            ? key.section(QChar('.'), -1)
            : key;

        QString normalized;
        if (validationKey == QStringLiteral("theme")) {
            normalized = value.toString().trimmed().toLower();
            if (!isValidTheme(normalized)) {
                continue;
            }
        } else if (validationKey == QStringLiteral("continuous_reading")
                   || validationKey == QStringLiteral("high_contrast")) {
            if (!value.isBool()) {
                continue;
            }
            normalized = value.toBool() ? QStringLiteral("1") : QStringLiteral("0");
        } else if (validationKey == QStringLiteral("font_size")
                   || validationKey == QStringLiteral("line_spacing")
                   || validationKey == QStringLiteral("speech_rate")
                   || validationKey == QStringLiteral("speech_volume")
                   || validationKey == QStringLiteral("reader_width")
                   || validationKey == QStringLiteral("reader_margin")) {
            if (!value.isDouble()) {
                continue;
            }
            const int number = value.toInt();
            if (validationKey == QStringLiteral("font_size") && (number < 12 || number > 32)) {
                continue;
            }
            if (validationKey == QStringLiteral("line_spacing") && (number < 100 || number > 220)) {
                continue;
            }
            if (validationKey == QStringLiteral("speech_rate") && (number < 50 || number > 200)) {
                continue;
            }
            if (validationKey == QStringLiteral("speech_volume") && (number < 0 || number > 100)) {
                continue;
            }
            if (validationKey == QStringLiteral("reader_width") && (number < 600 || number > 1400)) {
                continue;
            }
            if (validationKey == QStringLiteral("reader_margin") && (number < 12 || number > 80)) {
                continue;
            }
            normalized = QString::number(number);
        } else if (validationKey == QStringLiteral("font_family")) {
            normalized = value.toString().trimmed().left(64);
            if (normalized.isEmpty()) {
                continue;
            }
        } else if (validationKey == QStringLiteral("text_alignment")) {
            normalized = value.toString().trimmed().toLower();
            if (normalized != QStringLiteral("left") && normalized != QStringLiteral("center")
                && normalized != QStringLiteral("right") && normalized != QStringLiteral("justify")) {
                continue;
            }
        } else if (validationKey == QStringLiteral("image_fit")) {
            normalized = value.toString().trimmed().toLower();
            if (normalized != QStringLiteral("page") && normalized != QStringLiteral("width")
                && normalized != QStringLiteral("height") && normalized != QStringLiteral("actual")) {
                continue;
            }
        } else if (validationKey == QStringLiteral("reading_direction")) {
            normalized = value.toString().trimmed().toLower();
            if (normalized != QStringLiteral("ltr") && normalized != QStringLiteral("rtl")) {
                continue;
            }
        } else if (validationKey == QStringLiteral("custom_reader_background")
                   || validationKey == QStringLiteral("custom_reader_text")
                   || validationKey == QStringLiteral("custom_highlight_color")) {
            normalized = value.toString().trimmed().toLower();
            if (!isValidColor(normalized)) {
                continue;
            }
        }

        QSqlQuery query(connection);
        query.prepare(QStringLiteral(
            "INSERT INTO app_settings (key, value, updated_at) VALUES (:key, :value, CURRENT_TIMESTAMP) "
            "ON CONFLICT(key) DO UPDATE SET value = excluded.value, updated_at = CURRENT_TIMESTAMP"));
        query.bindValue(QStringLiteral(":key"), key);
        query.bindValue(QStringLiteral(":value"), normalized);
        if (!query.exec()) {
            connection.rollback();
            fail(QStringLiteral("The reading preferences restore could not be completed."));
            return false;
        }
        ++restored;
    }

    if (restored == 0 || !connection.commit()) {
        connection.rollback();
        fail(QStringLiteral("No valid reading preferences were found in this export."));
        return false;
    }
    lastError_.clear();
    emit stateChanged();
    return true;
}

bool BackupController::backupDatabase(const QString &path)
{
    if (!validatePath(path)) {
        return false;
    }

    const QString source = database_->connection().databaseName();
    if (QFileInfo(source).absoluteFilePath() == QFileInfo(path).absoluteFilePath()) {
        fail(QStringLiteral("Choose a different file for the backup."));
        return false;
    }
    if (QFileInfo::exists(path)) {
        fail(QStringLiteral("The backup file already exists. Choose a new file name."));
        return false;
    }

    QSqlQuery checkpoint(database_->connection());
    if (!checkpoint.exec(QStringLiteral("PRAGMA wal_checkpoint(FULL)"))) {
        fail(QStringLiteral("The local database could not be prepared for backup."));
        return false;
    }
    if (!QFile::copy(source, path)) {
        fail(QStringLiteral("The database backup could not be created."));
        return false;
    }
    lastError_.clear();
    emit stateChanged();
    return true;
}

bool BackupController::restoreDatabase(const QString &path)
{
    if (!validateSourcePath(path)) {
        return false;
    }

    const QString currentPath = database_->connection().databaseName();
    const QString sourcePath = QFileInfo(path).absoluteFilePath();
    if (currentPath.isEmpty() || QFileInfo(currentPath).absoluteFilePath() == sourcePath) {
        fail(QStringLiteral("Choose a database backup different from the active library."));
        return false;
    }

    QString validationError;
    if (!validateDatabaseFile(sourcePath, &validationError)) {
        fail(validationError);
        return false;
    }

    const QString currentAbsolutePath = QFileInfo(currentPath).absoluteFilePath();
    const QString temporaryPath = currentAbsolutePath
        + QStringLiteral(".restore-")
        + QUuid::createUuid().toString(QUuid::WithoutBraces)
        + QStringLiteral(".tmp");
    const QString previousPath = currentAbsolutePath
        + QStringLiteral(".previous-")
        + QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMddHHmmsszzz"))
        + QStringLiteral(".db");

    if (!QFile::copy(sourcePath, temporaryPath)) {
        fail(QStringLiteral("The database backup could not be staged for restoration."));
        return false;
    }

    QSqlQuery checkpoint(database_->connection());
    if (!checkpoint.exec(QStringLiteral("PRAGMA wal_checkpoint(TRUNCATE)"))) {
        QFile::remove(temporaryPath);
        fail(QStringLiteral("The active database could not be prepared for restoration."));
        return false;
    }
    emit databaseRestoreAboutToStart();
    database_->close();
    QFile::remove(currentAbsolutePath + QStringLiteral("-wal"));
    QFile::remove(currentAbsolutePath + QStringLiteral("-shm"));
    const auto restorePrevious = [&] {
        database_->close();
        QFile::remove(currentAbsolutePath);
        QFile::rename(previousPath, currentAbsolutePath);
        Lumina::Domain::Error reopenError;
        database_->open(currentAbsolutePath, &reopenError);
        database_->migrate(&reopenError);
    };

    if (!QFile::rename(currentAbsolutePath, previousPath)) {
        QFile::remove(temporaryPath);
        Lumina::Domain::Error reopenError;
        database_->open(currentAbsolutePath, &reopenError);
        database_->migrate(&reopenError);
        fail(QStringLiteral("The active database could not be prepared for restoration."));
        return false;
    }
    if (!QFile::rename(temporaryPath, currentAbsolutePath)) {
        QFile::rename(previousPath, currentAbsolutePath);
        Lumina::Domain::Error reopenError;
        database_->open(currentAbsolutePath, &reopenError);
        database_->migrate(&reopenError);
        fail(QStringLiteral("The database backup could not replace the active library."));
        return false;
    }

    Lumina::Domain::Error reopenError;
    if (!database_->open(currentAbsolutePath, &reopenError)
        || !database_->migrate(&reopenError)) {
        restorePrevious();
        fail(QStringLiteral("The restored database could not be opened. The previous library was kept."));
        return false;
    }

    lastError_.clear();
    emit databaseRestored();
    emit stateChanged();
    return true;
}

} // namespace Lumina::Application

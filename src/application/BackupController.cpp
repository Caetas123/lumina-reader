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
#include <QSqlQuery>
#include <QStringList>
#include <QTextStream>

#include <algorithm>

namespace {

bool isValidStatus(const QString &status)
{
    return status == QStringLiteral("to_read") || status == QStringLiteral("reading")
        || status == QStringLiteral("read") || status == QStringLiteral("abandoned");
}

bool isKnownSetting(const QString &key)
{
    static const QStringList keys = {
        QStringLiteral("theme"), QStringLiteral("font_size"),
        QStringLiteral("line_spacing"), QStringLiteral("continuous_reading"),
        QStringLiteral("high_contrast"), QStringLiteral("speech_rate"),
        QStringLiteral("speech_volume"),
    };
    return keys.contains(key);
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
            "description, language, is_favorite, rating, page_rotation "
            "FROM documents ORDER BY title COLLATE NOCASE"))) {
        fail(QStringLiteral("The library could not be read for export."));
        return false;
    }

    QJsonArray documents;
    while (query.next()) {
        documents.append(QJsonObject{
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
        });
    }

    const QJsonObject root{
        {QStringLiteral("format"), QStringLiteral("lumina-library")},
        {QStringLiteral("version"), 1},
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
        const QString value = query.value(1).toString();
        if (key == QStringLiteral("continuous_reading") || key == QStringLiteral("high_contrast")) {
            settings.insert(key, value == QStringLiteral("1"));
        } else if (key == QStringLiteral("font_size") || key == QStringLiteral("line_spacing")
                   || key == QStringLiteral("speech_rate") || key == QStringLiteral("speech_volume")) {
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
    if (!file.open(QIODevice::ReadOnly)) {
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
        || !root.value(QStringLiteral("documents")).isArray()) {
        fail(QStringLiteral("The selected file is not a Lumina library export."));
        return false;
    }

    QSqlDatabase connection = database_->connection();
    if (!connection.transaction()) {
        fail(QStringLiteral("The library restore could not be started."));
        return false;
    }

    int restored = 0;
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
        if (!hash.isEmpty()) {
            find.prepare(QStringLiteral(
                "SELECT id FROM documents WHERE file_hash = :hash "
                "OR file_path = :path OR canonical_path = :path LIMIT 1"));
            find.bindValue(QStringLiteral(":hash"), hash);
        } else {
            find.prepare(QStringLiteral(
                "SELECT id FROM documents WHERE file_path = :path OR canonical_path = :path LIMIT 1"));
        }
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

        QSqlQuery update(connection);
        update.prepare(QStringLiteral(
            "UPDATE documents SET title = :title, author = :author, description = :description, "
            "language = :language, status = :status, progress = :progress, current_page = :page, "
            "page_count = :page_count, last_opened_at = NULLIF(:last_opened, ''), "
            "is_favorite = :favorite, rating = :rating, page_rotation = :rotation, "
            "updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
        update.bindValue(QStringLiteral(":title"), item.value(QStringLiteral("title")).toString());
        update.bindValue(QStringLiteral(":author"), item.value(QStringLiteral("author")).toString());
        update.bindValue(QStringLiteral(":description"), item.value(QStringLiteral("description")).toString());
        update.bindValue(QStringLiteral(":language"), item.value(QStringLiteral("language")).toString());
        update.bindValue(QStringLiteral(":status"), restoredStatus);
        update.bindValue(QStringLiteral(":progress"), progress);
        update.bindValue(QStringLiteral(":page"), page);
        update.bindValue(QStringLiteral(":page_count"), pageCount);
        update.bindValue(QStringLiteral(":last_opened"), item.value(QStringLiteral("lastOpenedAt")).toString());
        update.bindValue(QStringLiteral(":favorite"), item.value(QStringLiteral("isFavorite")).toBool() ? 1 : 0);
        update.bindValue(QStringLiteral(":rating"), rating);
        update.bindValue(QStringLiteral(":rotation"), rotation);
        update.bindValue(QStringLiteral(":id"), find.value(0));
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
        search.bindValue(QStringLiteral(":id"), find.value(0));
        if (!search.exec()) {
            connection.rollback();
            fail(QStringLiteral("The library search index could not be restored."));
            return false;
        }
        ++restored;
    }

    if (!connection.commit()) {
        connection.rollback();
        fail(QStringLiteral("The library restore could not be saved."));
        return false;
    }
    if (restored == 0) {
        fail(QStringLiteral("No documents from this export matched the current library."));
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

        QString normalized;
        if (key == QStringLiteral("theme")) {
            normalized = value.toString().trimmed().toLower();
            if (!isValidTheme(normalized)) {
                continue;
            }
        } else if (key == QStringLiteral("continuous_reading")
                   || key == QStringLiteral("high_contrast")) {
            if (!value.isBool()) {
                continue;
            }
            normalized = value.toBool() ? QStringLiteral("1") : QStringLiteral("0");
        } else {
            if (!value.isDouble()) {
                continue;
            }
            const int number = value.toInt();
            if (key == QStringLiteral("font_size") && (number < 12 || number > 32)) {
                continue;
            }
            if (key == QStringLiteral("line_spacing") && (number < 100 || number > 220)) {
                continue;
            }
            if (key == QStringLiteral("speech_rate") && (number < 50 || number > 200)) {
                continue;
            }
            if (key == QStringLiteral("speech_volume") && (number < 0 || number > 100)) {
                continue;
            }
            normalized = QString::number(number);
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

} // namespace Lumina::Application

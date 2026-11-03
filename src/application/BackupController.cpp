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
#include <QTextStream>

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
            "status, progress, current_page, page_count, added_at, COALESCE(last_opened_at, '') "
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

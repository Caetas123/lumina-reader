#include "infrastructure/database/Database.h"

#include "infrastructure/logging/Logging.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QList>
#include <QStringList>
#include <QUuid>

namespace {

struct Migration {
    int version;
    QStringList statements;
};

QList<Migration> migrations()
{
    return {
        {
            1,
            {
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS app_settings ("
                    "key TEXT PRIMARY KEY NOT NULL, "
                    "value TEXT NOT NULL, "
                    "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                    ")"),
                QStringLiteral(
                    "CREATE VIRTUAL TABLE IF NOT EXISTS document_search USING fts5 ("
                    "document_id UNINDEXED, "
                    "title, "
                    "author, "
                    "description, "
                    "content"
                    ")"),
            },
        },
        {
            2,
            {
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS documents ("
                    "id TEXT PRIMARY KEY NOT NULL, "
                    "file_path TEXT NOT NULL UNIQUE, "
                    "canonical_path TEXT NOT NULL, "
                    "file_hash TEXT NOT NULL, "
                    "file_size INTEGER NOT NULL DEFAULT 0, "
                    "format TEXT NOT NULL, "
                    "content_type TEXT NOT NULL DEFAULT 'other', "
                    "title TEXT NOT NULL, "
                    "author TEXT NOT NULL DEFAULT '', "
                    "description TEXT NOT NULL DEFAULT '', "
                    "language TEXT NOT NULL DEFAULT '', "
                    "cover_path TEXT NOT NULL DEFAULT '', "
                    "status TEXT NOT NULL DEFAULT 'to_read', "
                    "progress REAL NOT NULL DEFAULT 0.0, "
                    "current_page INTEGER NOT NULL DEFAULT 0, "
                    "page_count INTEGER NOT NULL DEFAULT 0, "
                    "added_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                    "last_opened_at TEXT, "
                    "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                    "CHECK (progress >= 0.0 AND progress <= 1.0), "
                    "CHECK (current_page >= 0), "
                    "CHECK (page_count >= 0)"
                    ")"),
                QStringLiteral(
                    "CREATE INDEX IF NOT EXISTS documents_status_idx "
                    "ON documents(status)"),
                QStringLiteral(
                    "CREATE INDEX IF NOT EXISTS documents_last_opened_idx "
                    "ON documents(last_opened_at DESC)"),
                QStringLiteral(
                    "CREATE INDEX IF NOT EXISTS documents_hash_idx "
                    "ON documents(file_hash)"),
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS collections ("
                    "id TEXT PRIMARY KEY NOT NULL, "
                    "name TEXT NOT NULL UNIQUE, "
                    "description TEXT NOT NULL DEFAULT '', "
                    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                    "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                    ")"),
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS document_collections ("
                    "document_id TEXT NOT NULL REFERENCES documents(id) ON DELETE CASCADE, "
                    "collection_id TEXT NOT NULL REFERENCES collections(id) ON DELETE CASCADE, "
                    "PRIMARY KEY(document_id, collection_id)"
                    ")"),
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS tags ("
                    "id TEXT PRIMARY KEY NOT NULL, "
                    "name TEXT NOT NULL UNIQUE, "
                    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                    ")"),
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS document_tags ("
                    "document_id TEXT NOT NULL REFERENCES documents(id) ON DELETE CASCADE, "
                    "tag_id TEXT NOT NULL REFERENCES tags(id) ON DELETE CASCADE, "
                    "PRIMARY KEY(document_id, tag_id)"
                    ")"),
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS highlights ("
                    "id TEXT PRIMARY KEY NOT NULL, "
                    "document_id TEXT NOT NULL REFERENCES documents(id) ON DELETE CASCADE, "
                    "text TEXT NOT NULL, "
                    "note TEXT NOT NULL DEFAULT '', "
                    "color TEXT NOT NULL DEFAULT '#f6c945', "
                    "style TEXT NOT NULL DEFAULT 'highlight', "
                    "locator TEXT NOT NULL, "
                    "context_before TEXT NOT NULL DEFAULT '', "
                    "context_after TEXT NOT NULL DEFAULT '', "
                    "page INTEGER NOT NULL DEFAULT 0, "
                    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                    "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                    ")"),
                QStringLiteral(
                    "CREATE INDEX IF NOT EXISTS highlights_document_idx "
                    "ON highlights(document_id, page)"),
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS annotations ("
                    "id TEXT PRIMARY KEY NOT NULL, "
                    "document_id TEXT NOT NULL REFERENCES documents(id) ON DELETE CASCADE, "
                    "text TEXT NOT NULL, "
                    "kind TEXT NOT NULL DEFAULT 'note', "
                    "locator TEXT NOT NULL, "
                    "page INTEGER NOT NULL DEFAULT 0, "
                    "x REAL NOT NULL DEFAULT 0.0, "
                    "y REAL NOT NULL DEFAULT 0.0, "
                    "width REAL NOT NULL DEFAULT 0.0, "
                    "height REAL NOT NULL DEFAULT 0.0, "
                    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                    "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                    ")"),
                QStringLiteral(
                    "CREATE INDEX IF NOT EXISTS annotations_document_idx "
                    "ON annotations(document_id, page)"),
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS bookmarks ("
                    "id TEXT PRIMARY KEY NOT NULL, "
                    "document_id TEXT NOT NULL REFERENCES documents(id) ON DELETE CASCADE, "
                    "label TEXT NOT NULL DEFAULT '', "
                    "locator TEXT NOT NULL, "
                    "page INTEGER NOT NULL DEFAULT 0, "
                    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                    ")"),
                QStringLiteral(
                    "CREATE INDEX IF NOT EXISTS bookmarks_document_idx "
                    "ON bookmarks(document_id, page)"),
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS reading_history ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "document_id TEXT NOT NULL REFERENCES documents(id) ON DELETE CASCADE, "
                    "progress REAL NOT NULL, "
                    "page INTEGER NOT NULL DEFAULT 0, "
                    "recorded_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                    ")"),
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS ocr_pages ("
                    "document_id TEXT NOT NULL REFERENCES documents(id) ON DELETE CASCADE, "
                    "page INTEGER NOT NULL, "
                    "language TEXT NOT NULL DEFAULT '', "
                    "text TEXT NOT NULL DEFAULT '', "
                    "status TEXT NOT NULL DEFAULT 'pending', "
                    "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                    "PRIMARY KEY(document_id, page)"
                    ")"),
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS search_history ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "query TEXT NOT NULL, "
                    "scope TEXT NOT NULL, "
                    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                    ")"),
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS dictionary_history ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "term TEXT NOT NULL, "
                    "language TEXT NOT NULL DEFAULT '', "
                    "provider TEXT NOT NULL DEFAULT 'local', "
                    "result_json TEXT NOT NULL DEFAULT '{}', "
                    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                    ")"),
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS translation_history ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "source_text TEXT NOT NULL, "
                    "source_language TEXT NOT NULL DEFAULT '', "
                    "target_language TEXT NOT NULL, "
                    "provider TEXT NOT NULL DEFAULT 'local', "
                    "translated_text TEXT NOT NULL DEFAULT '', "
                    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                    ")"),
            },
        },
        {
            3,
            {
                QStringLiteral(
                    "CREATE VIRTUAL TABLE IF NOT EXISTS knowledge_search USING fts5 ("
                    "highlight_id UNINDEXED, "
                    "document_id UNINDEXED, "
                    "text, "
                    "note"
                    ")"),
                QStringLiteral(
                    "INSERT INTO knowledge_search (highlight_id, document_id, text, note) "
                    "SELECT id, document_id, text, note FROM highlights"),
            },
        },
        {
            4,
            {
                QStringLiteral(
                    "CREATE VIRTUAL TABLE IF NOT EXISTS knowledge_items_search USING fts5 ("
                    "source_id UNINDEXED, "
                    "source_kind UNINDEXED, "
                    "document_id UNINDEXED, "
                    "text, "
                    "note, "
                    "page UNINDEXED, "
                    "locator UNINDEXED, "
                    "created_at UNINDEXED"
                    ")"),
                QStringLiteral(
                    "INSERT INTO knowledge_items_search "
                    "(source_id, source_kind, document_id, text, note, page, locator, created_at) "
                    "SELECT id, 'highlight', document_id, text, note, page, locator, created_at "
                    "FROM highlights"),
                QStringLiteral(
                    "INSERT INTO knowledge_items_search "
                    "(source_id, source_kind, document_id, text, note, page, locator, created_at) "
                    "SELECT id, 'annotation', document_id, text, kind, page, locator, created_at "
                    "FROM annotations"),
            },
        },
        {
            5,
            {
                QStringLiteral(
                    "ALTER TABLE documents ADD COLUMN is_favorite INTEGER NOT NULL DEFAULT 0 "
                    "CHECK (is_favorite IN (0, 1))"),
                QStringLiteral(
                    "ALTER TABLE documents ADD COLUMN rating INTEGER NOT NULL DEFAULT 0 "
                    "CHECK (rating >= 0 AND rating <= 5)"),
                QStringLiteral(
                    "CREATE INDEX IF NOT EXISTS documents_favorite_idx "
                    "ON documents(is_favorite, last_opened_at DESC)"),
            },
        },
        {
            6,
            {
                QStringLiteral(
                    "INSERT INTO knowledge_items_search "
                    "(source_id, source_kind, document_id, text, note, page, locator, created_at) "
                    "SELECT document_id || ':' || page, 'ocr', document_id, text, '', page, "
                    "'ocr:' || page, updated_at FROM ocr_pages "
                    "WHERE status = 'completed' AND text <> ''"),
            },
        },
        {
            7,
            {
                QStringLiteral(
                    "ALTER TABLE documents ADD COLUMN page_rotation INTEGER NOT NULL DEFAULT 0 "
                    "CHECK (page_rotation IN (0, 90, 180, 270))"),
            },
        },
    };
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

Database::Database()
    : connectionName_(QStringLiteral("lumina-")
                      + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

Database::~Database()
{
    if (connection_.isValid()) {
        connection_.close();
        connection_ = {};
        QSqlDatabase::removeDatabase(connectionName_);
    }
}

bool Database::open(const QString &databasePath, Domain::Error *error)
{
    if (databasePath.isEmpty()) {
        setError(error,
                 Domain::ErrorCode::InvalidArgument,
                 QStringLiteral("Choose a valid location for the local database."),
                 QStringLiteral("Database path is empty."));
        return false;
    }

    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        setError(error,
                 Domain::ErrorCode::Database,
                 QStringLiteral("The local database driver is unavailable."),
                 QStringLiteral("Qt SQL driver QSQLITE is not available."));
        return false;
    }

    if (connection_.isValid()) {
        connection_.close();
        connection_ = {};
        QSqlDatabase::removeDatabase(connectionName_);
    }

    connection_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    connection_.setDatabaseName(databasePath);

    if (!connection_.open()) {
        const QString technicalMessage = connection_.lastError().text();
        setError(error,
                 Domain::ErrorCode::Database,
                 QStringLiteral("The local database could not be opened."),
                 technicalMessage);
        qCWarning(luminaDatabase) << technicalMessage;
        return false;
    }

    QSqlQuery pragmaQuery(connection_);
    if (!pragmaQuery.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
        const QString technicalMessage = pragmaQuery.lastError().text();
        setError(error,
                 Domain::ErrorCode::Database,
                 QStringLiteral("The local database could not be configured."),
                 technicalMessage);
        qCWarning(luminaDatabase) << technicalMessage;
        return false;
    }

    if (!pragmaQuery.exec(QStringLiteral("PRAGMA busy_timeout = 5000"))) {
        const QString technicalMessage = pragmaQuery.lastError().text();
        setError(error,
                 Domain::ErrorCode::Database,
                 QStringLiteral("The local database could not be configured."),
                 technicalMessage);
        qCWarning(luminaDatabase) << technicalMessage;
        return false;
    }

    return true;
}

bool Database::migrate(Domain::Error *error)
{
    if (!isOpen()) {
        setError(error,
                 Domain::ErrorCode::Database,
                 QStringLiteral("The local database is not open."),
                 QStringLiteral("Migration requested before database open."));
        return false;
    }

    QSqlQuery query(connection_);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS schema_migrations ("
            "version INTEGER PRIMARY KEY NOT NULL, "
            "applied_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ")"))) {
        const QString technicalMessage = query.lastError().text();
        setError(error,
                 Domain::ErrorCode::Migration,
                 QStringLiteral("The local database schema could not be prepared."),
                 technicalMessage);
        qCWarning(luminaDatabase) << technicalMessage;
        return false;
    }

    int currentVersion = 0;
    if (!query.exec(QStringLiteral("SELECT COALESCE(MAX(version), 0) FROM schema_migrations"))
        || !query.next()) {
        const QString technicalMessage = query.lastError().text();
        setError(error,
                 Domain::ErrorCode::Migration,
                 QStringLiteral("The local database version could not be read."),
                 technicalMessage);
        qCWarning(luminaDatabase) << technicalMessage;
        return false;
    }
    currentVersion = query.value(0).toInt();

    for (const Migration &migration : migrations()) {
        if (migration.version <= currentVersion) {
            continue;
        }

        if (!connection_.transaction()) {
            const QString technicalMessage = connection_.lastError().text();
            setError(error,
                     Domain::ErrorCode::Migration,
                     QStringLiteral("The local database migration could not start."),
                     technicalMessage);
            return false;
        }

        bool migrationSucceeded = true;
        for (const QString &statement : migration.statements) {
            QSqlQuery statementQuery(connection_);
            if (!statementQuery.exec(statement)) {
                const QString technicalMessage = statementQuery.lastError().text();
                setError(error,
                         Domain::ErrorCode::Migration,
                         QStringLiteral("The local database schema could not be updated."),
                         technicalMessage);
                qCWarning(luminaDatabase) << technicalMessage;
                migrationSucceeded = false;
                break;
            }
        }

        if (migrationSucceeded) {
            QSqlQuery versionQuery(connection_);
            versionQuery.prepare(QStringLiteral(
                "INSERT INTO schema_migrations (version) VALUES (:version)"));
            versionQuery.bindValue(QStringLiteral(":version"), migration.version);
            migrationSucceeded = versionQuery.exec();
            if (!migrationSucceeded) {
                const QString technicalMessage = versionQuery.lastError().text();
                setError(error,
                         Domain::ErrorCode::Migration,
                         QStringLiteral("The local database migration could not be recorded."),
                         technicalMessage);
                qCWarning(luminaDatabase) << technicalMessage;
            }
        }

        if (!migrationSucceeded || !connection_.commit()) {
            connection_.rollback();
            if (error == nullptr || !error->hasError()) {
                const QString technicalMessage = connection_.lastError().text();
                setError(error,
                         Domain::ErrorCode::Migration,
                         QStringLiteral("The local database migration could not be committed."),
                         technicalMessage);
            }
            return false;
        }

        currentVersion = migration.version;
        qCInfo(luminaDatabase) << "Applied schema migration" << currentVersion;
    }

    return true;
}

bool Database::isOpen() const
{
    return connection_.isValid() && connection_.isOpen();
}

int Database::schemaVersion() const
{
    if (!isOpen()) {
        return 0;
    }

    QSqlQuery query(connection_);
    if (!query.exec(QStringLiteral("SELECT COALESCE(MAX(version), 0) FROM schema_migrations"))
        || !query.next()) {
        return 0;
    }
    return query.value(0).toInt();
}

QSqlDatabase Database::connection() const
{
    return connection_;
}

} // namespace Lumina::Infrastructure

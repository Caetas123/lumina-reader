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

#include "infrastructure/database/Database.h"

#include <QSqlQuery>
#include <QtTest>

class DatabaseTest final : public QObject {
    Q_OBJECT

private slots:
    void appliesInitialMigration()
    {
        Lumina::Infrastructure::Database database;
        Lumina::Domain::Error error;

        QVERIFY2(database.open(QStringLiteral(":memory:"), &error),
                 qPrintable(error.logMessage()));
        QVERIFY2(database.migrate(&error), qPrintable(error.logMessage()));
        QCOMPARE(database.schemaVersion(), 7);

        QSqlQuery query(database.connection());
        QVERIFY(query.exec(QStringLiteral(
            "SELECT name FROM sqlite_master WHERE name IN "
            "('schema_migrations', 'app_settings', 'document_search', 'knowledge_search', "
            "'knowledge_items_search', 'documents', "
            "'highlights', 'annotations', 'bookmarks', 'ocr_pages')")));

        QStringList tableNames;
        while (query.next()) {
            tableNames.append(query.value(0).toString());
        }

        QVERIFY(tableNames.contains(QStringLiteral("schema_migrations")));
        QVERIFY(tableNames.contains(QStringLiteral("app_settings")));
        QVERIFY(tableNames.contains(QStringLiteral("document_search")));
        QVERIFY(tableNames.contains(QStringLiteral("knowledge_search")));
        QVERIFY(tableNames.contains(QStringLiteral("knowledge_items_search")));
        QVERIFY(tableNames.contains(QStringLiteral("documents")));
        QVERIFY(tableNames.contains(QStringLiteral("highlights")));
        QVERIFY(tableNames.contains(QStringLiteral("annotations")));
        QVERIFY(tableNames.contains(QStringLiteral("bookmarks")));
        QVERIFY(tableNames.contains(QStringLiteral("ocr_pages")));
    }

    void migrationIsIdempotent()
    {
        Lumina::Infrastructure::Database database;
        Lumina::Domain::Error error;

        QVERIFY2(database.open(QStringLiteral(":memory:"), &error),
                 qPrintable(error.logMessage()));
        QVERIFY2(database.migrate(&error), qPrintable(error.logMessage()));
        QVERIFY2(database.migrate(&error), qPrintable(error.logMessage()));
        QCOMPARE(database.schemaVersion(), 7);

        QSqlQuery query(database.connection());
        QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM schema_migrations")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 7);
    }

    void rejectsEmptyPath()
    {
        Lumina::Infrastructure::Database database;
        Lumina::Domain::Error error;

        QVERIFY(!database.open({}, &error));
        QVERIFY(error.hasError());
        QVERIFY(error.code() == Lumina::Domain::ErrorCode::InvalidArgument);
        QVERIFY(!error.userMessage().isEmpty());
    }
};

QTEST_GUILESS_MAIN(DatabaseTest)

#include "database_test.moc"

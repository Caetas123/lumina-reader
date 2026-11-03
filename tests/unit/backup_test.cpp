#include "application/BackupController.h"
#include "infrastructure/database/Database.h"
#include "infrastructure/library/LibraryStore.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class BackupTest final : public QObject {
    Q_OBJECT

private slots:
    void exportsKnowledgeAndLibrary()
    {
        Lumina::Infrastructure::Database database;
        Lumina::Domain::Error error;
        QVERIFY2(database.open(QStringLiteral(":memory:"), &error),
                 qPrintable(error.logMessage()));
        QVERIFY2(database.migrate(&error), qPrintable(error.logMessage()));

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString sourcePath = directory.filePath(QStringLiteral("notes.txt"));
        QFile source(sourcePath);
        QVERIFY(source.open(QIODevice::WriteOnly | QIODevice::Text));
        QVERIFY(source.write("A local idea worth remembering.") > 0);
        source.close();

        Lumina::Infrastructure::LibraryStore library;
        library.setDatabase(&database);
        QVERIFY(library.importFiles({sourcePath}));
        const QString documentId = library.data(
            library.index(0, 0), Lumina::Infrastructure::LibraryStore::IdRole).toString();
        QVERIFY(library.addHighlight(documentId,
                                     QStringLiteral("local idea"),
                                     QStringLiteral("text:0:2"),
                                     0,
                                     QStringLiteral("#f6c945"),
                                     QStringLiteral("Remember this")));
        QVERIFY(library.setFavorite(documentId, true));
        QVERIFY(library.setRating(documentId, 4));
        QVERIFY(library.addReadingTime(documentId, 90));
        QVERIFY(library.updateMetadata(documentId,
                                       QStringLiteral("Original title"),
                                       QStringLiteral("Original author"),
                                       QStringLiteral("A saved description"),
                                       QStringLiteral("en"),
                                       QStringLiteral("academic"),
                                       QStringLiteral("History")));

        Lumina::Application::BackupController backup;
        backup.setDatabase(&database);
        const QString markdownPath = directory.filePath(QStringLiteral("knowledge.md"));
        const QString jsonPath = directory.filePath(QStringLiteral("knowledge.json"));
        const QString libraryPath = directory.filePath(QStringLiteral("library.json"));
        const QString settingsPath = directory.filePath(QStringLiteral("settings.json"));
        QVERIFY(backup.exportKnowledge(markdownPath, QStringLiteral("markdown")));
        QVERIFY(backup.exportKnowledge(jsonPath, QStringLiteral("json")));
        QVERIFY(backup.exportLibrary(libraryPath));

        QSqlQuery settings(database.connection());
        settings.prepare(QStringLiteral(
            "INSERT INTO app_settings (key, value) VALUES ('theme', 'sepia') "
            "ON CONFLICT(key) DO UPDATE SET value = excluded.value"));
        QVERIFY(settings.exec());
        QVERIFY(backup.exportSettings(settingsPath));
        QVERIFY(settings.exec(QStringLiteral(
            "UPDATE app_settings SET value = 'dark' WHERE key = 'theme'")));
        QVERIFY(backup.restoreSettings(settingsPath));
        QVERIFY(settings.exec(QStringLiteral("SELECT value FROM app_settings WHERE key = 'theme'")));
        QVERIFY(settings.next());
        QCOMPARE(settings.value(0).toString(), QStringLiteral("sepia"));

        QVERIFY(library.updateMetadata(documentId,
                                       QStringLiteral("Changed title"),
                                       QStringLiteral("Changed author"),
                                       QStringLiteral("Changed description"),
                                       QStringLiteral("pt")));
        QVERIFY(library.updateMetadata(documentId,
                                       QStringLiteral("Temporary title"),
                                       QStringLiteral("Temporary author"),
                                       QStringLiteral("Temporary description"),
                                       QStringLiteral("es")));
        QVERIFY(backup.restoreLibrary(libraryPath));
        QCOMPARE(library.documentDetails(documentId).value(QStringLiteral("title")).toString(),
                 QStringLiteral("Original title"));
        QCOMPARE(library.documentDetails(documentId).value(QStringLiteral("language")).toString(),
                 QStringLiteral("en"));
        QCOMPARE(library.documentDetails(documentId).value(QStringLiteral("contentType")).toString(),
                 QStringLiteral("academic"));
        QCOMPARE(library.documentDetails(documentId).value(QStringLiteral("genre")).toString(),
                 QStringLiteral("History"));
        QVERIFY(library.documentDetails(documentId).value(QStringLiteral("isFavorite")).toBool());
        QCOMPARE(library.documentDetails(documentId).value(QStringLiteral("rating")).toInt(), 4);
        QCOMPARE(library.documentDetails(documentId).value(QStringLiteral("readingSeconds")).toLongLong(), 90);

        QFile markdown(markdownPath);
        QVERIFY(markdown.open(QIODevice::ReadOnly));
        QVERIFY(QString::fromUtf8(markdown.readAll()).contains(QStringLiteral("local idea")));

        QFile json(jsonPath);
        QVERIFY(json.open(QIODevice::ReadOnly));
        QVERIFY(QString::fromUtf8(json.readAll()).contains(QStringLiteral("lumina-knowledge")));

        QFile libraryFile(libraryPath);
        QVERIFY(libraryFile.open(QIODevice::ReadOnly));
        QVERIFY(QString::fromUtf8(libraryFile.readAll()).contains(QStringLiteral("notes")));
    }
};

QTEST_GUILESS_MAIN(BackupTest)

#include "backup_test.moc"

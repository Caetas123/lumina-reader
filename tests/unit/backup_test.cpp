#include "application/BackupController.h"
#include "infrastructure/database/Database.h"
#include "infrastructure/library/LibraryStore.h"

#include <QFile>
#include <QTemporaryDir>
#include <QSqlQuery>
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
                                     QStringLiteral("text:0:2:12"),
                                     0,
                                     QStringLiteral("#f6c945"),
                                     QStringLiteral("Remember this")));
        QVERIFY(library.setFavorite(documentId, true));
        QVERIFY(library.setRating(documentId, 4));
        QVERIFY(library.addReadingTime(documentId, 90));
        QVERIFY(library.updateProgress(documentId, 0.35, 2));
        QVERIFY(library.updateMetadata(documentId,
                                       QStringLiteral("Original title"),
                                       QStringLiteral("Original author"),
                                       QStringLiteral("A saved description"),
                                       QStringLiteral("en"),
                                       QStringLiteral("academic"),
                                       QStringLiteral("History")));
        QVERIFY(library.createCollection(QStringLiteral("Study"), QStringLiteral("Saved for study")));
        const QString collectionId = library.collections().first().toMap().value(QStringLiteral("id")).toString();
        QVERIFY(library.addToCollection(documentId, collectionId));
        QVERIFY(library.setTags(documentId, {QStringLiteral("history"), QStringLiteral("important")}));
        QVERIFY(library.addAnnotation(documentId,
                                      QStringLiteral("Review this note."),
                                      QStringLiteral("region"),
                                      QStringLiteral("region:0.1,0.2,0.3,0.4"),
                                      1,
                                      0.1,
                                      0.2,
                                      0.3,
                                      0.4));
        QVERIFY(library.addBookmark(documentId,
                                    QStringLiteral("Revisit"),
                                    QStringLiteral("text:0:10"),
                                    0));
        const QVariantList ocrWords{
            QVariantMap{{QStringLiteral("text"), QStringLiteral("Scanned")},
                        {QStringLiteral("start"), 0},
                        {QStringLiteral("length"), 7},
                        {QStringLiteral("x"), 0.1},
                        {QStringLiteral("y"), 0.2},
                        {QStringLiteral("width"), 0.3},
                        {QStringLiteral("height"), 0.1}},
        };
        QVERIFY(library.saveOcrPage(documentId,
                                    3,
                                    QStringLiteral("eng"),
                                    QStringLiteral("Scanned passage"),
                                    QStringLiteral("completed"),
                                    ocrWords));

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
        QVERIFY(settings.exec(QStringLiteral(
            "INSERT INTO app_settings (key, value) VALUES "
            "('reader.format.epub.font_size', '27'), "
            "('reader.type.book.high_contrast', '1'), "
            "('reader.document.doc-1.font_family', 'Georgia') "
            "ON CONFLICT(key) DO UPDATE SET value = excluded.value")));
        QVERIFY(backup.exportSettings(settingsPath));
        QVERIFY(settings.exec(QStringLiteral(
            "UPDATE app_settings SET value = 'dark' WHERE key = 'theme'")));
        QVERIFY(settings.exec(QStringLiteral(
            "UPDATE app_settings SET value = '18' WHERE key = 'reader.format.epub.font_size'")));
        QVERIFY(settings.exec(QStringLiteral(
            "UPDATE app_settings SET value = '0' WHERE key = 'reader.type.book.high_contrast'")));
        QVERIFY(settings.exec(QStringLiteral(
            "UPDATE app_settings SET value = 'Arial' WHERE key = 'reader.document.doc-1.font_family'")));
        QVERIFY(backup.restoreSettings(settingsPath));
        QVERIFY(settings.exec(QStringLiteral("SELECT value FROM app_settings WHERE key = 'theme'")));
        QVERIFY(settings.next());
        QCOMPARE(settings.value(0).toString(), QStringLiteral("sepia"));
        QVERIFY(settings.exec(QStringLiteral(
            "SELECT value FROM app_settings WHERE key = 'reader.format.epub.font_size'")));
        QVERIFY(settings.next());
        QCOMPARE(settings.value(0).toString(), QStringLiteral("27"));
        QVERIFY(settings.exec(QStringLiteral(
            "SELECT value FROM app_settings WHERE key = 'reader.type.book.high_contrast'")));
        QVERIFY(settings.next());
        QCOMPARE(settings.value(0).toString(), QStringLiteral("1"));
        QVERIFY(settings.exec(QStringLiteral(
            "SELECT value FROM app_settings WHERE key = 'reader.document.doc-1.font_family'")));
        QVERIFY(settings.next());
        QCOMPARE(settings.value(0).toString(), QStringLiteral("Georgia"));

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
        QVERIFY(settings.exec(QStringLiteral("DELETE FROM knowledge_items_search")));
        QVERIFY(settings.exec(QStringLiteral("DELETE FROM highlights")));
        QVERIFY(settings.exec(QStringLiteral("DELETE FROM annotations")));
        QVERIFY(settings.exec(QStringLiteral("DELETE FROM bookmarks")));
        QVERIFY(settings.exec(QStringLiteral("DELETE FROM ocr_pages")));
        QVERIFY(settings.exec(QStringLiteral("DELETE FROM reading_history")));
        QVERIFY(settings.exec(QStringLiteral("DELETE FROM document_collections")));
        QVERIFY(settings.exec(QStringLiteral("DELETE FROM document_tags")));
        QVERIFY(backup.restoreLibrary(libraryPath));
        library.refresh();
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
        QCOMPARE(library.documentDetails(documentId).value(QStringLiteral("currentPage")).toInt(), 2);
        QCOMPARE(library.highlightsForDocument(documentId).size(), 1);
        const QVariantMap restoredHighlight = library.highlightsForDocument(documentId).first().toMap();
        QVERIFY(!restoredHighlight.value(QStringLiteral("contextAfter")).toString().isEmpty());
        QCOMPARE(library.annotationsForDocument(documentId).size(), 2);
        QCOMPARE(library.bookmarksForDocument(documentId).size(), 1);
        QCOMPARE(library.tagsForDocument(documentId), QStringList({QStringLiteral("history"), QStringLiteral("important")}));
        QCOMPARE(library.collectionsForDocument(documentId).size(), 1);
        QCOMPARE(library.ocrTextForPage(documentId, 3), QStringLiteral("Scanned passage"));
        QCOMPARE(library.knowledgeSearch(QStringLiteral("Scanned")).size(), 1);

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

    void restoresDatabaseBackup()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString sourcePath = directory.filePath(QStringLiteral("source.db"));
        const QString targetPath = directory.filePath(QStringLiteral("target.db"));
        const QString backupPath = directory.filePath(QStringLiteral("lumina-backup.db"));
        const QString documentPath = directory.filePath(QStringLiteral("restored.txt"));
        QFile document(documentPath);
        QVERIFY(document.open(QIODevice::WriteOnly | QIODevice::Text));
        QVERIFY(document.write("Restored library content.") > 0);
        document.close();

        Lumina::Infrastructure::Database sourceDatabase;
        Lumina::Domain::Error error;
        QVERIFY2(sourceDatabase.open(sourcePath, &error), qPrintable(error.logMessage()));
        QVERIFY2(sourceDatabase.migrate(&error), qPrintable(error.logMessage()));
        Lumina::Infrastructure::LibraryStore sourceLibrary;
        sourceLibrary.setDatabase(&sourceDatabase);
        QVERIFY(sourceLibrary.importFiles({documentPath}));

        Lumina::Application::BackupController sourceBackup;
        sourceBackup.setDatabase(&sourceDatabase);
        QVERIFY(sourceBackup.backupDatabase(backupPath));

        Lumina::Infrastructure::Database targetDatabase;
        QVERIFY2(targetDatabase.open(targetPath, &error), qPrintable(error.logMessage()));
        QVERIFY2(targetDatabase.migrate(&error), qPrintable(error.logMessage()));
        Lumina::Application::BackupController targetBackup;
        targetBackup.setDatabase(&targetDatabase);
        QVERIFY(targetBackup.restoreDatabase(backupPath));

        QSqlQuery query(targetDatabase.connection());
        QVERIFY(query.exec(QStringLiteral("SELECT title FROM documents")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("restored"));
        QVERIFY(!query.next());
    }
};

QTEST_GUILESS_MAIN(BackupTest)

#include "backup_test.moc"

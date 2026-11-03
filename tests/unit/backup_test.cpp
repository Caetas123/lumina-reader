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

        Lumina::Application::BackupController backup;
        backup.setDatabase(&database);
        const QString markdownPath = directory.filePath(QStringLiteral("knowledge.md"));
        const QString jsonPath = directory.filePath(QStringLiteral("knowledge.json"));
        const QString libraryPath = directory.filePath(QStringLiteral("library.json"));
        QVERIFY(backup.exportKnowledge(markdownPath, QStringLiteral("markdown")));
        QVERIFY(backup.exportKnowledge(jsonPath, QStringLiteral("json")));
        QVERIFY(backup.exportLibrary(libraryPath));

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

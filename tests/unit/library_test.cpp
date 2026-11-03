#include "infrastructure/database/Database.h"
#include "infrastructure/library/LibraryStore.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class LibraryTest final : public QObject {
    Q_OBJECT

private slots:
    void importsTextAndRejectsDuplicate()
    {
        Lumina::Infrastructure::Database database;
        Lumina::Domain::Error error;
        QVERIFY2(database.open(QStringLiteral(":memory:"), &error),
                 qPrintable(error.logMessage()));
        QVERIFY2(database.migrate(&error), qPrintable(error.logMessage()));

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString filePath = directory.filePath(QStringLiteral("learning-notes.txt"));
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QVERIFY(file.write("Democracy is a practice of shared responsibility.\n") > 0);
        file.close();

        Lumina::Infrastructure::LibraryStore store;
        store.setDatabase(&database);
        QVERIFY(store.importFiles({filePath}));
        QCOMPARE(store.count(), 1);
        QVERIFY(store.importFiles({filePath}));
        QCOMPARE(store.count(), 1);

        const QModelIndex index = store.index(0, 0);
        const QString documentId = store.data(index, Lumina::Infrastructure::LibraryStore::IdRole)
                                       .toString();
        QCOMPARE(store.data(index, Lumina::Infrastructure::LibraryStore::FormatRole).toString(),
                 QStringLiteral("txt"));
        QCOMPARE(store.data(index, Lumina::Infrastructure::LibraryStore::TitleRole).toString(),
                 QStringLiteral("learning-notes"));

        QVERIFY(store.addHighlight(documentId,
                                   QStringLiteral("Democracy"),
                                   QStringLiteral("text:0:0"),
                                   0,
                                   QStringLiteral("#f6c945"),
                                   QStringLiteral("Core concept")));
        const QVariantList results = store.knowledgeSearch(QStringLiteral("democ"));
        QCOMPARE(results.size(), 1);
        QCOMPARE(results.first().toMap().value(QStringLiteral("documentId")).toString(), documentId);
        QCOMPARE(results.first().toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("highlight"));

        QVERIFY(store.addAnnotation(documentId,
                                     QStringLiteral("Review this argument later."),
                                     QStringLiteral("note"),
                                     QStringLiteral("text:0:20"),
                                     0));
        const QVariantList annotationResults = store.knowledgeSearch(QStringLiteral("argument"));
        QCOMPARE(annotationResults.size(), 1);
        QCOMPARE(annotationResults.first().toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("annotation"));
        QCOMPARE(store.annotationsForDocument(documentId).size(), 1);

        QVERIFY(store.addBookmark(documentId,
                                   QStringLiteral("Revisit"),
                                   QStringLiteral("text:0:20"),
                                   0));
        QCOMPARE(store.bookmarksForDocument(documentId).size(), 1);

        QVERIFY(store.updateProgress(documentId, 0.5, 2));
        QCOMPARE(store.documentDetails(documentId).value(QStringLiteral("status")).toString(),
                 QStringLiteral("reading"));
        QVERIFY(store.setStatus(documentId, QStringLiteral("read")));
        QCOMPARE(store.documentDetails(documentId).value(QStringLiteral("status")).toString(),
                 QStringLiteral("read"));

        QVERIFY(store.removeDocument(documentId));
        QCOMPARE(store.count(), 0);
    }
};

QTEST_GUILESS_MAIN(LibraryTest)

#include "library_test.moc"

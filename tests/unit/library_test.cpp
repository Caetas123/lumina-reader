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
        QVERIFY(!store.data(index, Lumina::Infrastructure::LibraryStore::FavoriteRole).toBool());
        QCOMPARE(store.data(index, Lumina::Infrastructure::LibraryStore::RatingRole).toInt(), 0);

        QVERIFY(store.setFavorite(documentId, true));
        QVERIFY(store.setRating(documentId, 5));
        QVERIFY(store.updateMetadata(documentId,
                                     QStringLiteral("Learning notes"),
                                     QStringLiteral("Lumina author"),
                                     QStringLiteral("Personal study notes"),
                                     QStringLiteral("en")));
        QCOMPARE(store.documentDetails(documentId).value(QStringLiteral("title")).toString(),
                 QStringLiteral("Learning notes"));
        QVERIFY(store.documentDetails(documentId).value(QStringLiteral("isFavorite")).toBool());
        QCOMPARE(store.documentDetails(documentId).value(QStringLiteral("rating")).toInt(), 5);

        QVERIFY(store.createCollection(QStringLiteral("Study"), QStringLiteral("Reading list")));
        const QVariantList collectionList = store.collections();
        QCOMPARE(collectionList.size(), 1);
        const QString collectionId = collectionList.first().toMap().value(QStringLiteral("id")).toString();
        QVERIFY(store.addToCollection(documentId, collectionId));
        QCOMPARE(store.collectionsForDocument(documentId).size(), 1);
        QVERIFY(store.setTags(documentId, {QStringLiteral("study"), QStringLiteral("important")}));
        QCOMPARE(store.tagsForDocument(documentId),
                 QStringList({QStringLiteral("important"), QStringLiteral("study")}));
        QVERIFY(store.removeFromCollection(documentId, collectionId));
        QCOMPARE(store.collectionsForDocument(documentId).size(), 0);
        QVERIFY(store.deleteCollection(collectionId));

        const QString secondPath = directory.filePath(QStringLiteral("second-note.txt"));
        QFile secondFile(secondPath);
        QVERIFY(secondFile.open(QIODevice::WriteOnly | QIODevice::Text));
        QVERIFY(secondFile.write("A second document for the folder import test.\n") > 0);
        secondFile.close();
        QVERIFY(store.importFolder(directory.path(), false));
        QCOMPARE(store.count(), 2);
        store.setViewFilter(QStringLiteral("favorites"));
        QCOMPARE(store.count(), 1);
        store.setViewFilter(QStringLiteral("all"));
        QCOMPARE(store.count(), 2);

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
        QCOMPARE(store.knowledgeSearch(QStringLiteral("democ"),
                                       QStringLiteral("txt"),
                                       QStringLiteral("#f6c945"))
                     .size(),
                 1);
        QCOMPARE(store.knowledgeSearch(QStringLiteral("democ"),
                                       QStringLiteral("epub"),
                                       QStringLiteral("#f6c945"))
                     .size(),
                 0);

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

        QVERIFY(store.addAnnotation(documentId,
                                     QStringLiteral("A selected image region."),
                                     QStringLiteral("region"),
                                     QStringLiteral("region:0.1,0.2,0.3,0.4"),
                                     1,
                                     0.1,
                                     0.2,
                                     0.3,
                                     0.4));
        const QVariantList annotations = store.annotationsForDocument(documentId);
        QCOMPARE(annotations.size(), 2);
        QCOMPARE(annotations.last().toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("region"));
        QCOMPARE(annotations.last().toMap().value(QStringLiteral("width")).toDouble(), 0.3);

        QVERIFY(store.addBookmark(documentId,
                                   QStringLiteral("Revisit"),
                                   QStringLiteral("text:0:20"),
                                   0));
        QCOMPARE(store.bookmarksForDocument(documentId).size(), 1);

        QVERIFY(store.updateProgress(documentId, 0.5, 2));
        QCOMPARE(store.documentDetails(documentId).value(QStringLiteral("status")).toString(),
                 QStringLiteral("reading"));
        store.setViewFilter(QStringLiteral("reading"));
        QCOMPARE(store.count(), 1);
        store.setViewFilter(QStringLiteral("all"));
        QVERIFY(store.setStatus(documentId, QStringLiteral("read")));
        QCOMPARE(store.documentDetails(documentId).value(QStringLiteral("status")).toString(),
                 QStringLiteral("read"));

        QVERIFY(store.removeDocument(documentId));
        QCOMPARE(store.count(), 1);
    }
};

QTEST_GUILESS_MAIN(LibraryTest)

#include "library_test.moc"

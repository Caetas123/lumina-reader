#include "infrastructure/database/Database.h"
#include "infrastructure/library/LibraryStore.h"

#include <QFile>
#include <QImage>
#include <QFileInfo>
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

        const QString movedPath = directory.filePath(QStringLiteral("moved-notes.txt"));
        QVERIFY(QFile::copy(filePath, movedPath));
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
        store.setViewFilter(QStringLiteral("collection:") + collectionId);
        QCOMPARE(store.count(), 1);
        store.setViewFilter(QStringLiteral("tag:study"));
        QCOMPARE(store.count(), 1);
        store.setViewFilter(QStringLiteral("type:text"));
        QCOMPARE(store.count(), 1);
        store.setViewFilter(QStringLiteral("all"));
        QVERIFY(store.updateMetadata(documentId,
                                     QStringLiteral("Learning notes"),
                                     QStringLiteral("Lumina author"),
                                     QStringLiteral("Personal study notes"),
                                     QStringLiteral("en"),
                                     QStringLiteral("academic")));
        QCOMPARE(store.documentDetails(documentId).value(QStringLiteral("contentType")).toString(),
                 QStringLiteral("academic"));
        QVERIFY(!store.updateMetadata(documentId,
                                      QStringLiteral("Learning notes"),
                                      QStringLiteral("Lumina author"),
                                      QStringLiteral("Personal study notes"),
                                      QStringLiteral("en"),
                                      QStringLiteral("unsupported-type")));
        QVERIFY(store.removeFromCollection(documentId, collectionId));
        QCOMPARE(store.collectionsForDocument(documentId).size(), 0);
        QVERIFY(store.deleteCollection(collectionId));

        const QString secondPath = directory.filePath(QStringLiteral("second-note.txt"));
        QFile secondFile(secondPath);
        QVERIFY(secondFile.open(QIODevice::WriteOnly | QIODevice::Text));
        QVERIFY(secondFile.write("A second document for the folder import test.\n") > 0);
        secondFile.close();
        QVERIFY(!store.relinkDocument(documentId, secondPath));
        QVERIFY(store.relinkDocument(documentId, movedPath));
        QCOMPARE(store.documentDetails(documentId).value(QStringLiteral("filePath")).toString(),
                 QFileInfo(movedPath).absoluteFilePath());
        QVERIFY(store.documentAvailable(documentId));
        QVERIFY(store.importFolder(directory.path(), false));
        QCOMPARE(store.count(), 2);
        store.setViewFilter(QStringLiteral("favorites"));
        QCOMPARE(store.count(), 1);
        store.setViewFilter(QStringLiteral("all"));
        QCOMPARE(store.count(), 2);
        const QString imagePath = directory.filePath(QStringLiteral("cover.png"));
        QImage image(32, 48, QImage::Format_ARGB32);
        image.fill(Qt::magenta);
        QVERIFY(image.save(imagePath));
        QVERIFY(store.importFiles({imagePath}));
        QString imageDocumentId;
        for (int row = 0; row < store.rowCount(); ++row) {
            const QModelIndex imageIndex = store.index(row, 0);
            if (store.data(imageIndex, Lumina::Infrastructure::LibraryStore::FormatRole).toString()
                == QStringLiteral("png")) {
                imageDocumentId = store.data(imageIndex, Lumina::Infrastructure::LibraryStore::IdRole)
                                      .toString();
                break;
            }
        }
        QVERIFY(!imageDocumentId.isEmpty());
        const QString generatedCover = store.documentDetails(imageDocumentId)
                                           .value(QStringLiteral("coverPath"))
                                           .toString();
        QVERIFY(!generatedCover.isEmpty());
        QVERIFY(QFileInfo::exists(generatedCover));
        QVERIFY(store.setCoverPath(imageDocumentId, imagePath));
        QCOMPARE(store.documentDetails(imageDocumentId).value(QStringLiteral("coverPath")).toString(),
                 QFileInfo(imagePath).absoluteFilePath());
        QVERIFY(store.setCoverPath(imageDocumentId, QStringLiteral("")));
        QVERIFY(store.documentDetails(imageDocumentId).value(QStringLiteral("coverPath")).toString().isEmpty());
        QVERIFY(store.setPageRotation(imageDocumentId, 90));
        QCOMPARE(store.documentDetails(imageDocumentId).value(QStringLiteral("pageRotation")).toInt(), 90);
        QVERIFY(!store.setPageRotation(imageDocumentId, 45));
        QVERIFY(store.removeDocument(imageDocumentId));

        QVERIFY(store.addHighlight(documentId,
                                   QStringLiteral("Democracy"),
                                   QStringLiteral("text:0:0"),
                                   0,
                                   QStringLiteral("#f6c945"),
                                   QStringLiteral("Core concept"),
                                   QStringLiteral("underline")));
        const QString highlightId = store.highlightsForDocument(documentId)
                                        .first().toMap().value(QStringLiteral("id")).toString();
        QCOMPARE(store.highlightsForDocument(documentId).first().toMap().value(QStringLiteral("style")).toString(),
                 QStringLiteral("underline"));
        QVERIFY(store.updateHighlight(highlightId,
                                      QStringLiteral("#7acb8a"),
                                      QStringLiteral("Updated concept"),
                                      QStringLiteral("strike")));
        const QVariantMap updatedHighlight = store.highlightsForDocument(documentId).first().toMap();
        QCOMPARE(updatedHighlight.value(QStringLiteral("color")).toString(), QStringLiteral("#7acb8a"));
        QCOMPARE(updatedHighlight.value(QStringLiteral("note")).toString(), QStringLiteral("Updated concept"));
        QCOMPARE(updatedHighlight.value(QStringLiteral("style")).toString(), QStringLiteral("strike"));
        QVERIFY(!store.updateHighlight(highlightId,
                                       QStringLiteral("#7acb8a"),
                                       QStringLiteral("Invalid"),
                                       QStringLiteral("dotted")));
        const QVariantList results = store.knowledgeSearch(QStringLiteral("democ"));
        QCOMPARE(results.size(), 1);
        QCOMPARE(results.first().toMap().value(QStringLiteral("documentId")).toString(), documentId);
        QCOMPARE(results.first().toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("highlight"));
        QCOMPARE(results.first().toMap().value(QStringLiteral("note")).toString(),
                 QStringLiteral("Updated concept"));
        QCOMPARE(store.knowledgeSearch(QStringLiteral("democ"),
                                       QStringLiteral("txt"),
                                       QStringLiteral("#7acb8a"))
                     .size(),
                 1);
        QCOMPARE(store.knowledgeSearch(QStringLiteral("democ"),
                                       QStringLiteral("epub"),
                                       QStringLiteral("#7acb8a"))
                     .size(),
                 0);
        QCOMPARE(store.knowledgeDocumentOptions().size(), 2);
        QCOMPARE(store.knowledgeSearch(QStringLiteral("democ"),
                                       QStringLiteral(""),
                                       QStringLiteral(""),
                                       documentId,
                                       QStringLiteral("Lumina author"))
                     .size(),
                 1);
        QCOMPARE(store.knowledgeSearch(QStringLiteral("democ"),
                                       QStringLiteral(""),
                                       QStringLiteral(""),
                                       QStringLiteral(""),
                                       QStringLiteral("Different author"))
                     .size(),
                 0);
        QCOMPARE(store.knowledgeSearch(QStringLiteral("democ"),
                                       QStringLiteral(""),
                                       QStringLiteral(""),
                                       QStringLiteral(""),
                                       QStringLiteral(""),
                                       QStringLiteral("2000-01-01"))
                     .size(),
                 1);
        QCOMPARE(store.knowledgeSearch(QStringLiteral("democ"),
                                       QStringLiteral(""),
                                       QStringLiteral(""),
                                       QStringLiteral(""),
                                       QStringLiteral(""),
                                       QStringLiteral(""),
                                       QStringLiteral("2000-01-01"))
                     .size(),
                 0);

        QVERIFY(store.saveOcrPage(documentId,
                                  3,
                                  QStringLiteral("eng"),
                                  QStringLiteral("Scanned democracy passage")));
        QCOMPARE(store.ocrTextForPage(documentId, 3), QStringLiteral("Scanned democracy passage"));
        const QVariantList ocrResults = store.knowledgeSearch(QStringLiteral("scanned"));
        QCOMPARE(ocrResults.size(), 1);
        QCOMPARE(ocrResults.first().toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("ocr"));
        QCOMPARE(ocrResults.first().toMap().value(QStringLiteral("page")).toInt(), 3);

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
        QVERIFY(store.addReadingTime(documentId, 125));
        QCOMPARE(store.documentDetails(documentId).value(QStringLiteral("readingSeconds")).toLongLong(),
                 125);
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

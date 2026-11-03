#include "infrastructure/database/Database.h"
#include "infrastructure/library/LibraryStore.h"
#include "infrastructure/text/TextDecoder.h"

#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

class PerformanceTest final : public QObject {
    Q_OBJECT

private slots:
    void benchmarksLibraryRefreshAndKnowledgeSearch()
    {
        Lumina::Infrastructure::Database database;
        Lumina::Domain::Error error;
        QVERIFY2(database.open(QStringLiteral(":memory:"), &error),
                 qPrintable(error.logMessage()));
        QVERIFY2(database.migrate(&error), qPrintable(error.logMessage()));

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QStringList paths;
        for (int index = 0; index < 32; ++index) {
            const QString path = directory.filePath(QStringLiteral("document-%1.txt").arg(index));
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
            const QByteArray content = QByteArrayLiteral("Democracy and reading are useful study topic ")
                + QByteArray::number(index) + QByteArrayLiteral(".\n");
            QVERIFY(file.write(content) == content.size());
            file.close();
            paths.append(path);
        }

        Lumina::Infrastructure::LibraryStore library;
        library.setDatabase(&database);
        QVERIFY(library.importFiles(paths));
        QCOMPARE(library.count(), paths.size());
        for (int row = 0; row < library.count(); ++row) {
            const QString documentId = library.data(
                library.index(row, 0), Lumina::Infrastructure::LibraryStore::IdRole).toString();
            QVERIFY(library.addHighlight(documentId,
                                         QStringLiteral("Democracy"),
                                         QStringLiteral("text:0:0:10"),
                                         0,
                                         QStringLiteral("#f6c945"),
                                         QString()));
        }
        QVERIFY(!library.knowledgeSearch(QStringLiteral("democracy")).isEmpty());

        QBENCHMARK {
            library.refresh();
        }

        QBENCHMARK {
            const QVariantList results = library.knowledgeSearch(QStringLiteral("democracy"));
            Q_UNUSED(results)
        }
    }

    void benchmarksTextAndImageReading()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString textPath = directory.filePath(QStringLiteral("large.txt"));
        QFile textFile(textPath);
        QVERIFY(textFile.open(QIODevice::WriteOnly | QIODevice::Text));
        const QByteArray paragraph("Lumina Reader keeps study material local and searchable.\n");
        for (int index = 0; index < 4096; ++index) {
            QVERIFY(textFile.write(paragraph) == paragraph.size());
        }
        textFile.close();

        const QString imagePath = directory.filePath(QStringLiteral("page.png"));
        QImage source(QSize(1200, 1800), QImage::Format_ARGB32_Premultiplied);
        source.fill(Qt::white);
        QVERIFY(source.save(imagePath));

        QBENCHMARK {
            const QString text = Lumina::Infrastructure::TextDecoder::readFile(
                textPath, QStringLiteral("txt"));
            Q_UNUSED(text)
        }

        QBENCHMARK {
            const QImage image(imagePath);
            Q_UNUSED(image)
        }
    }
};

QTEST_GUILESS_MAIN(PerformanceTest)

#include "performance_test.moc"

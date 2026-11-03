#include "application/ReaderController.h"
#include "infrastructure/database/Database.h"
#include "infrastructure/library/LibraryStore.h"
#include "infrastructure/text/TextDecoder.h"

#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QtTest>

class ReaderTest final : public QObject {
    Q_OBJECT

private slots:
    void paginatesTextAndPersistsPosition()
    {
        Lumina::Infrastructure::Database database;
        Lumina::Domain::Error error;
        QVERIFY2(database.open(QStringLiteral(":memory:"), &error),
                 qPrintable(error.logMessage()));
        QVERIFY2(database.migrate(&error), qPrintable(error.logMessage()));

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString filePath = directory.filePath(QStringLiteral("long-note.txt"));
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QVERIFY(file.write(QByteArray(5000, 'A') + QByteArray("PAGE TWO")) > 0);
        file.close();

        Lumina::Infrastructure::LibraryStore library;
        library.setDatabase(&database);
        QVERIFY(library.importFiles({filePath}));
        const QString documentId = library.data(
            library.index(0, 0), Lumina::Infrastructure::LibraryStore::IdRole).toString();

        Lumina::Application::ReaderController reader(&library);
        QVERIFY(reader.openDocument(documentId));
        QCOMPARE(reader.pageCount(), 2);
        QCOMPARE(reader.currentPage(), 0);
        QCOMPARE(library.documentDetails(documentId).value(QStringLiteral("pageCount")).toInt(), 2);
        QVERIFY(!reader.startDocumentOcr(QStringLiteral("eng")));
        QCOMPARE(reader.ocrStatus(),
                 QStringLiteral("This document has no image pages that need OCR."));
        QVERIFY(!reader.textContent().contains(QStringLiteral("PAGE TWO")));
        QVERIFY(reader.continuousTextContent().contains(QStringLiteral("PAGE TWO")));
        QVERIFY(reader.pageTextAvailable());
        const QString exportPath = directory.filePath(QStringLiteral("exported.txt"));
        QVERIFY(reader.exportText(exportPath));
        QFile exported(exportPath);
        QVERIFY(exported.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY(QString::fromUtf8(exported.readAll()).contains(QStringLiteral("PAGE TWO")));
        QVERIFY(!reader.exportText(filePath));
        QCOMPARE(reader.errorMessage(),
                 QStringLiteral("Choose a different destination from the original document."));

        QVERIFY(reader.nextPage());
        QCOMPARE(reader.currentPage(), 1);
        QVERIFY(reader.textContent().contains(QStringLiteral("PAGE TWO")));
        QVERIFY(reader.findInDocument(QStringLiteral("page two")));
        QCOMPARE(reader.searchMatches(), 1);
        QVERIFY(reader.searchStart() >= 0);
        QCOMPARE(library.documentDetails(documentId).value(QStringLiteral("currentPage")).toInt(), 1);
    }

    void decodesRtfAndFb2Text()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString rtfPath = directory.filePath(QStringLiteral("note.rtf"));
        QFile rtf(rtfPath);
        QVERIFY(rtf.open(QIODevice::WriteOnly | QIODevice::Text));
        QVERIFY(rtf.write("{\\rtf1\\ansi First\\par Second}") > 0);
        rtf.close();

        const QString fb2Path = directory.filePath(QStringLiteral("book.fb2"));
        QFile fb2(fb2Path);
        QVERIFY(fb2.open(QIODevice::WriteOnly | QIODevice::Text));
        QVERIFY(fb2.write("<FictionBook><body><section><title>Chapter</title><p>Text from FB2.</p></section></body></FictionBook>") > 0);
        fb2.close();

        QCOMPARE(Lumina::Infrastructure::TextDecoder::readFile(rtfPath, QStringLiteral("rtf")),
                 QStringLiteral("First\nSecond"));
        QVERIFY(Lumina::Infrastructure::TextDecoder::readFile(fb2Path, QStringLiteral("fb2"))
                    .contains(QStringLiteral("Text from FB2.")));
    }

    void rendersImageThumbnail()
    {
        Lumina::Infrastructure::Database database;
        Lumina::Domain::Error error;
        QVERIFY2(database.open(QStringLiteral(":memory:"), &error),
                 qPrintable(error.logMessage()));
        QVERIFY2(database.migrate(&error), qPrintable(error.logMessage()));

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString imagePath = directory.filePath(QStringLiteral("page.png"));
        QImage image(320, 180, QImage::Format_ARGB32);
        image.fill(Qt::blue);
        QVERIFY(image.save(imagePath));

        Lumina::Infrastructure::LibraryStore library;
        library.setDatabase(&database);
        QVERIFY(library.importFiles({imagePath}));
        const QString documentId = library.data(
            library.index(0, 0), Lumina::Infrastructure::LibraryStore::IdRole).toString();

        Lumina::Application::ReaderController reader(&library);
        QVERIFY(reader.openDocument(documentId));
        QVERIFY(reader.isImageDocument());
        const QString thumbnail = reader.thumbnailSource(0);
        QVERIFY(thumbnail.startsWith(QStringLiteral("data:image/png;base64,")));
        QVERIFY(reader.thumbnailSource(-1).isEmpty());
    }

    void decodesUnencryptedPalmDoc()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("book.mobi"));
        QByteArray record(16, '\0');
        record[1] = 1;
        record[8] = 0;
        record[9] = 1;
        record[10] = 0x10;
        record[11] = 0;
        record.append("PalmDOC text");

        QByteArray data(86, '\0');
        data[76] = 0;
        data[77] = 1;
        data[78] = 0;
        data[79] = 0;
        data[80] = 0;
        data[81] = 86;
        data.append(record);
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write(data) == data.size());
        file.close();

        QCOMPARE(Lumina::Infrastructure::TextDecoder::readFile(path, QStringLiteral("mobi")),
                 QStringLiteral("PalmDOC text"));
    }

    void rejectsMalformedPalmDocRecordTable()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("malformed.mobi"));

        QByteArray data(94, '\0');
        data[76] = 0;
        data[77] = 2;
        data[78] = 0;
        data[79] = 0;
        data[80] = 0;
        data[81] = 94;
        data[86] = 0;
        data[87] = 0;
        data[88] = 0;
        data[89] = 93;
        data.append(QByteArray(16, '\0'));

        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write(data) == data.size());
        file.close();

        QVERIFY(Lumina::Infrastructure::TextDecoder::readFile(path, QStringLiteral("mobi")).isEmpty());
    }
};

QTEST_GUILESS_MAIN(ReaderTest)

#include "reader_test.moc"

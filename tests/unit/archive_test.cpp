#include "infrastructure/archive/ArchiveReader.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#ifdef LUMINA_HAS_QT_ZIP
#include <QtCore/private/qzipwriter_p.h>
#endif

class ArchiveTest final : public QObject {
    Q_OBJECT

private slots:
    void readsComicPages()
    {
#ifndef LUMINA_HAS_QT_ZIP
        QSKIP("Qt archive support is unavailable in this build.");
#else
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("sample.cbz"));

        QZipWriter writer(path);
        writer.addFile(QStringLiteral("02.jpg"), QByteArray("page two"));
        writer.addFile(QStringLiteral("01.jpg"), QByteArray("page one"));
        writer.close();

        Lumina::Infrastructure::ArchiveReader reader;
        QVERIFY(reader.open(path, QStringLiteral("cbz")));
        QCOMPARE(reader.pageCount(), 2);
        QCOMPARE(reader.pageName(0), QStringLiteral("01.jpg"));
        QCOMPARE(reader.pageData(1), QByteArray("page two"));
        QVERIFY(reader.pageIsImage(0));
#endif
    }

    void readsEpubSpine()
    {
#ifndef LUMINA_HAS_QT_ZIP
        QSKIP("Qt archive support is unavailable in this build.");
#else
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("sample.epub"));

        QZipWriter writer(path);
        writer.addFile(
            QStringLiteral("META-INF/container.xml"),
            QByteArray("<container><rootfiles><rootfile full-path=\"OEBPS/content.opf\"/></rootfiles></container>"));
        writer.addFile(
            QStringLiteral("OEBPS/content.opf"),
            QByteArray("<package><manifest><item id=\"chapter\" href=\"chapter.xhtml\" "
                       "media-type=\"application/xhtml+xml\"/></manifest><spine>"
                       "<itemref idref=\"chapter\"/></spine></package>"));
        writer.addFile(QStringLiteral("OEBPS/chapter.xhtml"),
                       QByteArray("<html><body><h1>Chapter one</h1><p>Local reading.</p></body></html>"));
        writer.close();

        Lumina::Infrastructure::ArchiveReader reader;
        QVERIFY(reader.open(path, QStringLiteral("epub")));
        QCOMPARE(reader.pageCount(), 1);
        QVERIFY(reader.pageText(0).contains(QStringLiteral("Chapter one")));
        QVERIFY(!reader.pageIsImage(0));
#endif
    }

    void readsTarComicPages()
    {
#ifndef LUMINA_HAS_LIBARCHIVE
        QSKIP("libarchive support is unavailable in this build.");
#else
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("sample.cbt"));
        const QByteArray payload("page one");

        QByteArray header(512, '\0');
        const auto writeField = [&header](int offset, int length, const QByteArray &value) {
            const int bytes = qMin(length, value.size());
            if (bytes > 0) {
                header.replace(offset, bytes, value.constData(), bytes);
            }
        };
        writeField(0, 100, QByteArrayLiteral("page.png"));
        writeField(100, 8, QByteArrayLiteral("0000644"));
        writeField(108, 8, QByteArrayLiteral("0000000"));
        writeField(116, 8, QByteArrayLiteral("0000000"));
        QByteArray sizeField = QByteArray::number(payload.size(), 8).rightJustified(11, '0');
        sizeField.append('\0');
        writeField(124, 12, sizeField);
        writeField(136, 12, QByteArrayLiteral("00000000000"));
        for (int index = 148; index < 156; ++index) {
            header[index] = ' ';
        }
        writeField(156, 1, QByteArrayLiteral("0"));
        writeField(257, 6, QByteArrayLiteral("ustar"));
        writeField(263, 2, QByteArrayLiteral("00"));

        quint32 checksum = 0;
        for (const char byte : header) {
            checksum += static_cast<unsigned char>(byte);
        }
        QByteArray checksumField = QByteArray::number(checksum, 8).rightJustified(6, '0');
        checksumField.append('\0');
        checksumField.append(' ');
        writeField(148, 8, checksumField);

        QByteArray archive = header;
        archive.append(payload);
        archive.append(QByteArray((512 - (payload.size() % 512)) % 512, '\0'));
        archive.append(QByteArray(1024, '\0'));

        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(archive), archive.size());
        file.close();

        Lumina::Infrastructure::ArchiveReader reader;
        QVERIFY(reader.open(path, QStringLiteral("cbt")));
        QCOMPARE(reader.pageCount(), 1);
        QCOMPARE(reader.pageName(0), QStringLiteral("page.png"));
        QCOMPARE(reader.pageData(0), payload);
        QVERIFY(reader.pageIsImage(0));
#endif
    }

    void rejectsPathTraversal()
    {
#ifndef LUMINA_HAS_QT_ZIP
        QSKIP("Qt archive support is unavailable in this build.");
#else
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("unsafe.epub"));

        QZipWriter writer(path);
        writer.addFile(QStringLiteral("META-INF/container.xml"),
                       QByteArray("<container><rootfiles><rootfile full-path=\"OEBPS/content.opf\"/></rootfiles></container>"));
        writer.addFile(QStringLiteral("OEBPS/content.opf"),
                       QByteArray("<package><manifest><item id=\"chapter\" href=\"../chapter.xhtml\" "
                                  "media-type=\"application/xhtml+xml\"/></manifest><spine>"
                                  "<itemref idref=\"chapter\"/></spine></package>"));
        writer.addFile(QStringLiteral("chapter.xhtml"), QByteArray("<p>outside</p>"));
        writer.close();

        Lumina::Infrastructure::ArchiveReader reader;
        QVERIFY(!reader.open(path, QStringLiteral("epub")));
#endif
    }
};

QTEST_GUILESS_MAIN(ArchiveTest)

#include "archive_test.moc"

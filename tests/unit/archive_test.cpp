#include "infrastructure/archive/ArchiveReader.h"

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

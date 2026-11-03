#include "application/ReaderController.h"
#include "infrastructure/database/Database.h"
#include "infrastructure/library/LibraryStore.h"

#include <QFile>
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
        QVERIFY(!reader.textContent().contains(QStringLiteral("PAGE TWO")));

        QVERIFY(reader.nextPage());
        QCOMPARE(reader.currentPage(), 1);
        QVERIFY(reader.textContent().contains(QStringLiteral("PAGE TWO")));
        QVERIFY(reader.findInDocument(QStringLiteral("page two")));
        QCOMPARE(reader.searchMatches(), 1);
        QVERIFY(reader.searchStart() >= 0);
        QCOMPARE(library.documentDetails(documentId).value(QStringLiteral("currentPage")).toInt(), 1);
    }
};

QTEST_GUILESS_MAIN(ReaderTest)

#include "reader_test.moc"

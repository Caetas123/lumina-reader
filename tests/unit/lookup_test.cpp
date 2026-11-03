#include "application/LookupController.h"

#include <QtTest>

class LookupTest final : public QObject {
    Q_OBJECT

private slots:
    void rejectsEmptyRequests()
    {
        Lumina::Application::LookupController controller;

        QVERIFY(!controller.lookupDictionary(QStringLiteral(" ")));
        QCOMPARE(controller.lastError(), QStringLiteral("Select a word first."));
        QVERIFY(!controller.translateText(QStringLiteral("\n")));
        QCOMPARE(controller.lastError(), QStringLiteral("Select a word or passage first."));
        QVERIFY(!controller.openDictionary(QStringLiteral("")));
        QVERIFY(!controller.openTranslation(QStringLiteral("")));
    }
};

QTEST_GUILESS_MAIN(LookupTest)

#include "lookup_test.moc"

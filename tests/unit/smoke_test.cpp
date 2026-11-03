#include <QtTest>

class SmokeTest final : public QObject {
    Q_OBJECT

private slots:
    void applicationVersionIsDefined()
    {
        QVERIFY(!QStringLiteral(LUMINA_VERSION).isEmpty());
    }
};

QTEST_APPLESS_MAIN(SmokeTest)

#include "smoke_test.moc"

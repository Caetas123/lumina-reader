#include "application/SettingsController.h"
#include "application/SpeechController.h"
#include "infrastructure/database/Database.h"

#include <QSqlQuery>
#include <QtTest>

class SettingsTest final : public QObject {
    Q_OBJECT

private slots:
    void persistsReadingPreferences()
    {
        Lumina::Infrastructure::Database database;
        Lumina::Domain::Error error;
        QVERIFY2(database.open(QStringLiteral(":memory:"), &error),
                 qPrintable(error.logMessage()));
        QVERIFY2(database.migrate(&error), qPrintable(error.logMessage()));

        Lumina::Application::SettingsController settings;
        settings.setDatabase(&database);
        QCOMPARE(settings.theme(), QStringLiteral("light"));
        QCOMPARE(settings.fontSize(), 18);

        settings.setTheme(QStringLiteral("sepia"));
        settings.setFontSize(26);
        settings.setLineSpacing(160);
        settings.setContinuousReading(false);
        settings.setHighContrast(true);

        Lumina::Application::SpeechController speech;
        speech.setDatabase(&database);
        speech.setRate(150);
        speech.setVolume(70);

        Lumina::Application::SettingsController reloaded;
        reloaded.setDatabase(&database);
        QCOMPARE(reloaded.theme(), QStringLiteral("sepia"));
        QCOMPARE(reloaded.fontSize(), 26);
        QCOMPARE(reloaded.lineSpacing(), 160);
        QVERIFY(!reloaded.continuousReading());
        QVERIFY(reloaded.highContrast());

        Lumina::Application::SpeechController reloadedSpeech;
        reloadedSpeech.setDatabase(&database);
        QCOMPARE(reloadedSpeech.rate(), 150);
        QCOMPARE(reloadedSpeech.volume(), 70);

        QSqlQuery query(database.connection());
        QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM app_settings")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 7);
    }
};

QTEST_GUILESS_MAIN(SettingsTest)

#include "settings_test.moc"

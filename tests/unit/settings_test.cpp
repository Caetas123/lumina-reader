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
        QCOMPARE(settings.theme(), QStringLiteral("dark"));
        QCOMPARE(settings.fontSize(), 18);

        settings.setTheme(QStringLiteral("sepia"));
        settings.setFontSize(26);
        settings.setLineSpacing(160);
        settings.setContinuousReading(false);
        settings.setHighContrast(true);
        settings.setFontFamily(QStringLiteral("Georgia"));
        settings.setReaderWidth(1040);
        settings.setReaderMargin(44);
        settings.setTextAlignment(QStringLiteral("justify"));
        settings.setImageFit(QStringLiteral("width"));
        settings.setReadingDirection(QStringLiteral("rtl"));
        settings.setCustomReaderBackground(QStringLiteral("#112233"));
        settings.setCustomReaderText(QStringLiteral("#fefefe"));
        settings.setCustomHighlightColor(QStringLiteral("#aabbcc"));

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
        QCOMPARE(reloaded.fontFamily(), QStringLiteral("Georgia"));
        QCOMPARE(reloaded.readerWidth(), 1040);
        QCOMPARE(reloaded.readerMargin(), 44);
        QCOMPARE(reloaded.textAlignment(), QStringLiteral("justify"));
        QCOMPARE(reloaded.imageFit(), QStringLiteral("width"));
        QCOMPARE(reloaded.readingDirection(), QStringLiteral("rtl"));
        QCOMPARE(reloaded.customReaderBackground(), QStringLiteral("#112233"));
        QCOMPARE(reloaded.customReaderText(), QStringLiteral("#fefefe"));
        QCOMPARE(reloaded.customHighlightColor(), QStringLiteral("#aabbcc"));

        Lumina::Application::SpeechController reloadedSpeech;
        reloadedSpeech.setDatabase(&database);
        QCOMPARE(reloadedSpeech.rate(), 150);
        QCOMPARE(reloadedSpeech.volume(), 70);

        QSqlQuery query(database.connection());
        QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM app_settings")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 16);
    }

    void resolvesScopedReaderPreferences()
    {
        Lumina::Infrastructure::Database database;
        Lumina::Domain::Error error;
        QVERIFY2(database.open(QStringLiteral(":memory:"), &error),
                 qPrintable(error.logMessage()));
        QVERIFY2(database.migrate(&error), qPrintable(error.logMessage()));

        Lumina::Application::SettingsController settings;
        settings.setDatabase(&database);
        settings.setReaderPreferenceValue(QStringLiteral("font_family"),
                                          QStringLiteral("global"),
                                          QStringLiteral("epub"),
                                          QStringLiteral("book"),
                                          QStringLiteral("doc-1"),
                                          QStringLiteral("Georgia"));
        settings.setReaderPreferenceValue(QStringLiteral("font_family"),
                                          QStringLiteral("format"),
                                          QStringLiteral("epub"),
                                          QStringLiteral("book"),
                                          QStringLiteral("doc-1"),
                                          QStringLiteral("Atkinson Hyperlegible"));
        settings.setReaderPreferenceValue(QStringLiteral("font_family"),
                                          QStringLiteral("type"),
                                          QStringLiteral("epub"),
                                          QStringLiteral("book"),
                                          QStringLiteral("doc-1"),
                                          QStringLiteral("Verdana"));
        settings.setReaderPreferenceValue(QStringLiteral("font_family"),
                                          QStringLiteral("document"),
                                          QStringLiteral("epub"),
                                          QStringLiteral("book"),
                                          QStringLiteral("doc-1"),
                                          QStringLiteral("Inter"));

        QCOMPARE(settings.readerPreferenceValue(QStringLiteral("font_family"),
                                                QStringLiteral("epub"),
                                                QStringLiteral("book"),
                                                QStringLiteral("doc-1"),
                                                QStringLiteral("Segoe UI")),
                 QStringLiteral("Inter"));
        QCOMPARE(settings.readerPreferenceValue(QStringLiteral("font_family"),
                                                QStringLiteral("epub"),
                                                QStringLiteral("book"),
                                                QStringLiteral("doc-2"),
                                                QStringLiteral("Segoe UI")),
                 QStringLiteral("Verdana"));
        QCOMPARE(settings.readerPreferenceValue(QStringLiteral("font_family"),
                                                QStringLiteral("epub"),
                                                QStringLiteral("magazine"),
                                                QStringLiteral("doc-3"),
                                                QStringLiteral("Segoe UI")),
                 QStringLiteral("Atkinson Hyperlegible"));
        QCOMPARE(settings.readerPreferenceValue(QStringLiteral("font_family"),
                                                QStringLiteral("pdf"),
                                                QStringLiteral("magazine"),
                                                QStringLiteral("doc-4"),
                                                QStringLiteral("Segoe UI")),
                 QStringLiteral("Georgia"));
    }

    void persistsNamedProfilesWithoutChangingDefaultPreferences()
    {
        Lumina::Infrastructure::Database database;
        Lumina::Domain::Error error;
        QVERIFY2(database.open(QStringLiteral(":memory:"), &error),
                 qPrintable(error.logMessage()));
        QVERIFY2(database.migrate(&error), qPrintable(error.logMessage()));

        Lumina::Application::SettingsController settings;
        settings.setDatabase(&database);
        const QStringList defaultProfiles{QStringLiteral("Default")};
        QCOMPARE(settings.profileNames(), defaultProfiles);
        QCOMPARE(settings.activeProfile(), QStringLiteral("Default"));
        settings.setTheme(QStringLiteral("dark"));
        settings.setReaderPreferenceValue(QStringLiteral("font_size"),
                                          QStringLiteral("global"),
                                          QStringLiteral("txt"),
                                          QStringLiteral("text"),
                                          QStringLiteral("doc-1"),
                                          QStringLiteral("18"));

        QVERIFY(settings.createProfile(QStringLiteral("Study mode")));
        QCOMPARE(settings.activeProfile(), QStringLiteral("Study mode"));
        const QStringList namedProfiles{QStringLiteral("Default"), QStringLiteral("Study mode")};
        QCOMPARE(settings.profileNames(), namedProfiles);
        settings.setTheme(QStringLiteral("sepia"));
        settings.setReaderPreferenceValue(QStringLiteral("font_size"),
                                          QStringLiteral("global"),
                                          QStringLiteral("txt"),
                                          QStringLiteral("text"),
                                          QStringLiteral("doc-1"),
                                          QStringLiteral("24"));

        settings.setActiveProfile(QStringLiteral("Default"));
        QCOMPARE(settings.theme(), QStringLiteral("dark"));
        QCOMPARE(settings.readerPreferenceValue(QStringLiteral("font_size"),
                                                QStringLiteral("txt"),
                                                QStringLiteral("text"),
                                                QStringLiteral("doc-1"),
                                                QStringLiteral("12")),
                 QStringLiteral("18"));

        settings.setActiveProfile(QStringLiteral("Study mode"));
        QCOMPARE(settings.theme(), QStringLiteral("sepia"));
        QCOMPARE(settings.readerPreferenceValue(QStringLiteral("font_size"),
                                                QStringLiteral("txt"),
                                                QStringLiteral("text"),
                                                QStringLiteral("doc-1"),
                                                QStringLiteral("12")),
                 QStringLiteral("24"));

        Lumina::Application::SettingsController reloaded;
        reloaded.setDatabase(&database);
        QCOMPARE(reloaded.activeProfile(), QStringLiteral("Study mode"));
        QCOMPARE(reloaded.theme(), QStringLiteral("sepia"));
        QVERIFY(reloaded.deleteProfile(QStringLiteral("Study mode")));
        QCOMPARE(reloaded.activeProfile(), QStringLiteral("Default"));
        QCOMPARE(reloaded.profileNames(), defaultProfiles);
    }
};

QTEST_GUILESS_MAIN(SettingsTest)

#include "settings_test.moc"

#include "application/LookupController.h"
#include "infrastructure/database/Database.h"

#include <QSqlQuery>
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

    void persistsAndManagesHistory()
    {
        Lumina::Infrastructure::Database database;
        Lumina::Domain::Error error;
        QVERIFY2(database.open(QStringLiteral(":memory:"), &error),
                 qPrintable(error.logMessage()));
        QVERIFY2(database.migrate(&error), qPrintable(error.logMessage()));

        QSqlQuery insert(database.connection());
        QVERIFY(insert.exec(QStringLiteral(
            "INSERT INTO dictionary_history "
            "(term, language, provider, result_json) "
            "VALUES ('luminous', 'en', 'test', '{}')")));
        const qint64 dictionaryId = insert.lastInsertId().toLongLong();
        QVERIFY(insert.exec(QStringLiteral(
            "INSERT INTO translation_history "
            "(source_text, source_language, target_language, provider, translated_text) "
            "VALUES ('hello', 'en', 'pt', 'test', 'olá')")));
        const qint64 translationId = insert.lastInsertId().toLongLong();

        Lumina::Application::LookupController controller;
        controller.setDatabase(&database);
        QCOMPARE(controller.dictionaryHistory().size(), 1);
        QCOMPARE(controller.translationHistory().size(), 1);
        QCOMPARE(controller.translationHistory().first().toMap()
                     .value(QStringLiteral("translatedText")).toString(),
                 QStringLiteral("olá"));

        QVERIFY(controller.setDictionaryFavorite(dictionaryId, true));
        QVERIFY(controller.dictionaryHistory().first().toMap()
                    .value(QStringLiteral("favorite")).toBool());
        QVERIFY(controller.setTranslationFavorite(translationId, true));
        QVERIFY(controller.translationHistory().first().toMap()
                    .value(QStringLiteral("favorite")).toBool());

        QVERIFY(controller.clearDictionaryHistory());
        QVERIFY(controller.clearTranslationHistory());
        QCOMPARE(controller.dictionaryHistory().size(), 0);
        QCOMPARE(controller.translationHistory().size(), 0);
    }

    void usesSavedLookupResultsOffline()
    {
        Lumina::Infrastructure::Database database;
        Lumina::Domain::Error error;
        QVERIFY2(database.open(QStringLiteral(":memory:"), &error),
                 qPrintable(error.logMessage()));
        QVERIFY2(database.migrate(&error), qPrintable(error.logMessage()));

        QSqlQuery insert(database.connection());
        insert.prepare(QStringLiteral(
            "INSERT INTO dictionary_history (term, language, provider, result_json) "
            "VALUES (:term, :language, :provider, :result)"));
        insert.bindValue(QStringLiteral(":term"), QStringLiteral("luminous"));
        insert.bindValue(QStringLiteral(":language"), QStringLiteral("en"));
        insert.bindValue(QStringLiteral(":provider"), QStringLiteral("test"));
        insert.bindValue(QStringLiteral(":result"), QStringLiteral(
            "[{\"meanings\":[{\"partOfSpeech\":\"adjective\","
            "\"definitions\":[{\"definition\":\"giving off light\"}]}]}]"));
        QVERIFY(insert.exec());
        insert.prepare(QStringLiteral(
            "INSERT INTO translation_history "
            "(source_text, source_language, target_language, provider, translated_text) "
            "VALUES (:text, :source, :target, :provider, :translated)"));
        insert.bindValue(QStringLiteral(":text"), QStringLiteral("hello"));
        insert.bindValue(QStringLiteral(":source"), QStringLiteral("en"));
        insert.bindValue(QStringLiteral(":target"), QStringLiteral("pt"));
        insert.bindValue(QStringLiteral(":provider"), QStringLiteral("test"));
        insert.bindValue(QStringLiteral(":translated"), QStringLiteral("olá"));
        QVERIFY(insert.exec());

        Lumina::Application::LookupController controller;
        controller.setDatabase(&database);
        QVERIFY(controller.lookupDictionary(QStringLiteral("luminous"), QStringLiteral("en")));
        QVERIFY(controller.resultBody().contains(QStringLiteral("giving off light")));
        QVERIFY(controller.resultSource().startsWith(QStringLiteral("Saved locally")));
        QVERIFY(controller.translateText(QStringLiteral("hello"), QStringLiteral("en"), QStringLiteral("pt")));
        QCOMPARE(controller.resultBody(), QStringLiteral("olá"));
        QVERIFY(controller.resultSource().startsWith(QStringLiteral("Saved locally")));
    }
};

QTEST_GUILESS_MAIN(LookupTest)

#include "lookup_test.moc"

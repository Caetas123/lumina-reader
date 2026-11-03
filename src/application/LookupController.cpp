#include "application/LookupController.h"

#include "domain/lookup/LookupProvider.h"
#include "infrastructure/database/Database.h"

#include <QDesktopServices>
#include <QSqlQuery>
#include <QUrlQuery>

namespace {

class GoogleDictionaryProvider final : public Lumina::Domain::DictionaryProvider {
public:
    QString id() const override
    {
        return QStringLiteral("google-dictionary");
    }

    QUrl lookupUrl(const QString &term, const QString &language) const override
    {
        Q_UNUSED(language)
        QUrl url(QStringLiteral("https://www.google.com/search"));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("q"), QStringLiteral("define: ") + term);
        url.setQuery(query);
        return url;
    }
};

class GoogleTranslationProvider final : public Lumina::Domain::TranslationProvider {
public:
    QString id() const override
    {
        return QStringLiteral("google-translate");
    }

    QUrl translationUrl(const QString &text,
                       const QString &sourceLanguage,
                       const QString &targetLanguage) const override
    {
        QUrl url(QStringLiteral("https://translate.google.com/"));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("sl"), sourceLanguage.isEmpty() ? QStringLiteral("auto") : sourceLanguage);
        query.addQueryItem(QStringLiteral("tl"), targetLanguage.isEmpty() ? QStringLiteral("en") : targetLanguage);
        query.addQueryItem(QStringLiteral("text"), text);
        url.setQuery(query);
        return url;
    }
};

} // namespace

namespace Lumina::Application {

LookupController::LookupController(QObject *parent)
    : QObject(parent)
{
}

QString LookupController::lastError() const
{
    return lastError_;
}

void LookupController::setDatabase(Lumina::Infrastructure::Database *database)
{
    database_ = database;
}

void LookupController::fail(const QString &message)
{
    lastError_ = message;
    emit stateChanged();
}

bool LookupController::openUrl(const QUrl &url)
{
    lastError_.clear();
    if (!url.isValid() || url.scheme() != QStringLiteral("https")) {
        fail(QStringLiteral("The selected lookup could not be opened."));
        return false;
    }
    if (!QDesktopServices::openUrl(url)) {
        fail(QStringLiteral("The default browser could not be opened."));
        return false;
    }
    emit stateChanged();
    return true;
}

bool LookupController::recordDictionaryLookup(const QString &term, const QString &language)
{
    if (database_ == nullptr || !database_->isOpen()) {
        return true;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "INSERT INTO dictionary_history (term, language, provider, result_json) "
        "VALUES (:term, :language, 'google-dictionary', '{}')"));
    query.bindValue(QStringLiteral(":term"), term);
    query.bindValue(QStringLiteral(":language"), language);
    return query.exec();
}

bool LookupController::recordTranslationLookup(const QString &text,
                                               const QString &sourceLanguage,
                                               const QString &targetLanguage)
{
    if (database_ == nullptr || !database_->isOpen()) {
        return true;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "INSERT INTO translation_history (source_text, source_language, target_language, provider) "
        "VALUES (:text, :source, :target, 'google-translate')"));
    query.bindValue(QStringLiteral(":text"), text);
    query.bindValue(QStringLiteral(":source"), sourceLanguage);
    query.bindValue(QStringLiteral(":target"), targetLanguage);
    return query.exec();
}

bool LookupController::openGoogle(const QString &text)
{
    const QString term = text.trimmed();
    if (term.isEmpty()) {
        fail(QStringLiteral("Select a word or passage first."));
        return false;
    }
    QUrl url(QStringLiteral("https://www.google.com/search"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("q"), term);
    url.setQuery(query);
    return openUrl(url);
}

bool LookupController::openDictionary(const QString &term, const QString &language)
{
    const QString normalized = term.trimmed();
    if (normalized.isEmpty()) {
        fail(QStringLiteral("Select a word first."));
        return false;
    }
    GoogleDictionaryProvider provider;
    if (!recordDictionaryLookup(normalized, language)) {
        fail(QStringLiteral("The lookup history could not be saved."));
        return false;
    }
    return openUrl(provider.lookupUrl(normalized, language));
}

bool LookupController::openTranslation(const QString &text,
                                       const QString &sourceLanguage,
                                       const QString &targetLanguage)
{
    const QString normalized = text.trimmed();
    if (normalized.isEmpty()) {
        fail(QStringLiteral("Select a word or passage first."));
        return false;
    }
    GoogleTranslationProvider provider;
    if (!recordTranslationLookup(normalized, sourceLanguage, targetLanguage)) {
        fail(QStringLiteral("The translation history could not be saved."));
        return false;
    }
    return openUrl(provider.translationUrl(normalized, sourceLanguage, targetLanguage));
}

} // namespace Lumina::Application

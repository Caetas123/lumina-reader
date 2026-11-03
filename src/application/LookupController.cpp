#include "application/LookupController.h"

#include "domain/lookup/LookupProvider.h"
#include "infrastructure/database/Database.h"

#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSqlQuery>
#include <QStringList>
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

class DictionaryApiProvider final : public Lumina::Domain::DictionaryProvider {
public:
    QString id() const override
    {
        return QStringLiteral("dictionaryapi.dev");
    }

    QUrl lookupUrl(const QString &term, const QString &language) const override
    {
        const QString normalizedLanguage = language.trimmed().isEmpty()
            ? QStringLiteral("en")
            : language.trimmed().toLower();
        const QString encodedTerm = QString::fromUtf8(QUrl::toPercentEncoding(term.trimmed()));
        return QUrl(QStringLiteral("https://api.dictionaryapi.dev/api/v2/entries/%1/%2")
                        .arg(normalizedLanguage, encodedTerm));
    }
};

class MyMemoryTranslationProvider final : public Lumina::Domain::TranslationProvider {
public:
    QString id() const override
    {
        return QStringLiteral("mymemory.translated.net");
    }

    QUrl translationUrl(const QString &text,
                       const QString &sourceLanguage,
                       const QString &targetLanguage) const override
    {
        QUrl url(QStringLiteral("https://api.mymemory.translated.net/get"));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("q"), text);
        query.addQueryItem(QStringLiteral("langpair"),
                           (sourceLanguage.trimmed().isEmpty() ? QStringLiteral("auto") : sourceLanguage)
                               + QStringLiteral("|")
                               + (targetLanguage.trimmed().isEmpty() ? QStringLiteral("en") : targetLanguage));
        url.setQuery(query);
        return url;
    }
};

QString normalizedLanguage(const QString &language, const QString &fallback)
{
    const QString normalized = language.trimmed().toLower();
    if (normalized.isEmpty()) {
        return fallback;
    }
    static const QRegularExpression languagePattern(QStringLiteral("^(auto|[a-z]{2,3}(?:-[a-z]{2,4})?)$"));
    return languagePattern.match(normalized).hasMatch() ? normalized : fallback;
}

QString dictionaryBody(const QJsonArray &entries, QString *pronunciation)
{
    QStringList sections;
    for (const QJsonValue &entryValue : entries) {
        const QJsonObject entry = entryValue.toObject();
        const QJsonArray phonetics = entry.value(QStringLiteral("phonetics")).toArray();
        if (pronunciation != nullptr && pronunciation->isEmpty()) {
            for (const QJsonValue &phoneticValue : phonetics) {
                const QString phonetic = phoneticValue.toObject().value(QStringLiteral("text")).toString().trimmed();
                if (!phonetic.isEmpty()) {
                    *pronunciation = phonetic;
                    break;
                }
            }
        }

        const QJsonArray meanings = entry.value(QStringLiteral("meanings")).toArray();
        for (const QJsonValue &meaningValue : meanings) {
            const QJsonObject meaning = meaningValue.toObject();
            const QString partOfSpeech = meaning.value(QStringLiteral("partOfSpeech")).toString().trimmed();
            QStringList lines;
            if (!partOfSpeech.isEmpty()) {
                lines << QStringLiteral("%1").arg(partOfSpeech);
            }
            const QJsonArray definitions = meaning.value(QStringLiteral("definitions")).toArray();
            int definitionCount = 0;
            for (const QJsonValue &definitionValue : definitions) {
                const QJsonObject definition = definitionValue.toObject();
                const QString definitionText = definition.value(QStringLiteral("definition")).toString().trimmed();
                if (definitionText.isEmpty()) {
                    continue;
                }
                QString line = QStringLiteral("• %1").arg(definitionText);
                const QString example = definition.value(QStringLiteral("example")).toString().trimmed();
                if (!example.isEmpty()) {
                    line += QStringLiteral("\n  %1").arg(example);
                }
                lines << line;
                if (++definitionCount >= 6) {
                    break;
                }
            }
            const QJsonArray synonyms = meaning.value(QStringLiteral("synonyms")).toArray();
            if (!synonyms.isEmpty()) {
                QStringList words;
                for (const QJsonValue &synonym : synonyms) {
                    if (words.size() >= 8) {
                        break;
                    }
                    words << synonym.toString();
                }
                if (!words.isEmpty()) {
                    lines << QStringLiteral("Synonyms: %1").arg(words.join(QStringLiteral(", ")));
                }
            }
            if (!lines.isEmpty()) {
                sections << lines.join(QStringLiteral("\n"));
            }
        }
    }
    return sections.join(QStringLiteral("\n\n"));
}

} // namespace

namespace Lumina::Application {

LookupController::LookupController(QObject *parent)
    : QObject(parent)
    , networkManager_(this)
{
}

QString LookupController::lastError() const
{
    return lastError_;
}

bool LookupController::busy() const
{
    return busy_;
}

QString LookupController::resultTitle() const
{
    return resultTitle_;
}

QString LookupController::resultBody() const
{
    return resultBody_;
}

QString LookupController::resultPronunciation() const
{
    return resultPronunciation_;
}

QString LookupController::resultSource() const
{
    return resultSource_;
}

void LookupController::setDatabase(Lumina::Infrastructure::Database *database)
{
    database_ = database;
}

void LookupController::clearResult()
{
    resultTitle_.clear();
    resultBody_.clear();
    resultPronunciation_.clear();
    resultSource_.clear();
}

void LookupController::fail(const QString &message)
{
    busy_ = false;
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

bool LookupController::recordDictionaryLookup(const QString &term,
                                              const QString &language,
                                              const QString &provider,
                                              const QString &resultJson)
{
    if (database_ == nullptr || !database_->isOpen()) {
        return true;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "INSERT INTO dictionary_history (term, language, provider, result_json) "
        "VALUES (:term, :language, :provider, :result)"));
    query.bindValue(QStringLiteral(":term"), term);
    query.bindValue(QStringLiteral(":language"), language);
    query.bindValue(QStringLiteral(":provider"), provider);
    query.bindValue(QStringLiteral(":result"), resultJson);
    return query.exec();
}

bool LookupController::recordTranslationLookup(const QString &text,
                                               const QString &sourceLanguage,
                                               const QString &targetLanguage,
                                               const QString &provider)
{
    if (database_ == nullptr || !database_->isOpen()) {
        return true;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "INSERT INTO translation_history (source_text, source_language, target_language, provider) "
        "VALUES (:text, :source, :target, :provider)"));
    query.bindValue(QStringLiteral(":text"), text);
    query.bindValue(QStringLiteral(":source"), sourceLanguage);
    query.bindValue(QStringLiteral(":target"), targetLanguage);
    query.bindValue(QStringLiteral(":provider"), provider);
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
    if (!recordDictionaryLookup(normalized,
                                language,
                                provider.id())) {
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
    if (!recordTranslationLookup(normalized,
                                 sourceLanguage,
                                 targetLanguage,
                                 provider.id())) {
        fail(QStringLiteral("The translation history could not be saved."));
        return false;
    }
    return openUrl(provider.translationUrl(normalized, sourceLanguage, targetLanguage));
}

bool LookupController::lookupDictionary(const QString &term, const QString &language)
{
    const QString normalizedTerm = term.trimmed();
    if (normalizedTerm.isEmpty()) {
        fail(QStringLiteral("Select a word first."));
        return false;
    }
    if (activeReply_ != nullptr) {
        QNetworkReply *previous = activeReply_.data();
        activeReply_.clear();
        previous->abort();
        previous->deleteLater();
    }

    DictionaryApiProvider provider;
    const QString languageCode = normalizedLanguage(language, QStringLiteral("en"));
    QNetworkRequest request(provider.lookupUrl(normalizedTerm, languageCode));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("LuminaReader/%1").arg(LUMINA_VERSION));
    activeReply_ = networkManager_.get(request);
    busy_ = true;
    lastError_.clear();
    clearResult();
    emit stateChanged();

    QPointer<QNetworkReply> reply = activeReply_;
    connect(reply, &QNetworkReply::finished, this, [this, reply, normalizedTerm, languageCode, provider]() {
        if (reply == nullptr || reply != activeReply_) {
            if (reply != nullptr) {
                reply->deleteLater();
            }
            return;
        }
        activeReply_.clear();
        const QByteArray payload = reply->read(2 * 1024 * 1024 + 1);
        const QNetworkReply::NetworkError networkError = reply->error();
        const QString errorString = reply->errorString();
        reply->deleteLater();
        if (networkError != QNetworkReply::NoError) {
            fail(QStringLiteral("The online dictionary could not be reached: %1").arg(errorString));
            return;
        }
        if (payload.size() > 2 * 1024 * 1024) {
            fail(QStringLiteral("The dictionary response was too large to display safely."));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
            fail(QStringLiteral("The dictionary returned an unreadable response."));
            return;
        }
        QString pronunciation;
        const QString body = dictionaryBody(document.array(), &pronunciation);
        if (body.isEmpty()) {
            fail(QStringLiteral("No definition was found for this word."));
            return;
        }
        if (!recordDictionaryLookup(normalizedTerm,
                                    languageCode,
                                    provider.id(),
                                    QString::fromUtf8(payload))) {
            fail(QStringLiteral("The dictionary result could not be saved locally."));
            return;
        }
        busy_ = false;
        resultTitle_ = normalizedTerm;
        resultBody_ = body;
        resultPronunciation_ = pronunciation;
        resultSource_ = QStringLiteral("Dictionary API · %1").arg(languageCode);
        lastError_.clear();
        emit stateChanged();
    });
    return true;
}

bool LookupController::translateText(const QString &text,
                                     const QString &sourceLanguage,
                                     const QString &targetLanguage)
{
    const QString normalizedText = text.trimmed();
    if (normalizedText.isEmpty()) {
        fail(QStringLiteral("Select a word or passage first."));
        return false;
    }
    if (activeReply_ != nullptr) {
        QNetworkReply *previous = activeReply_.data();
        activeReply_.clear();
        previous->abort();
        previous->deleteLater();
    }

    MyMemoryTranslationProvider provider;
    const QString normalizedSource = normalizedLanguage(sourceLanguage, QStringLiteral("auto"));
    const QString normalizedTarget = normalizedLanguage(targetLanguage, QStringLiteral("en"));
    QNetworkRequest request(provider.translationUrl(normalizedText,
                                                    normalizedSource,
                                                    normalizedTarget));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("LuminaReader/%1").arg(LUMINA_VERSION));
    activeReply_ = networkManager_.get(request);
    busy_ = true;
    lastError_.clear();
    clearResult();
    emit stateChanged();

    QPointer<QNetworkReply> reply = activeReply_;
    connect(reply, &QNetworkReply::finished, this, [this, reply, normalizedText, normalizedSource, normalizedTarget, provider]() {
        if (reply == nullptr || reply != activeReply_) {
            if (reply != nullptr) {
                reply->deleteLater();
            }
            return;
        }
        activeReply_.clear();
        const QByteArray payload = reply->read(2 * 1024 * 1024 + 1);
        const QNetworkReply::NetworkError networkError = reply->error();
        const QString errorString = reply->errorString();
        reply->deleteLater();
        if (networkError != QNetworkReply::NoError) {
            fail(QStringLiteral("The translation service could not be reached: %1").arg(errorString));
            return;
        }
        if (payload.size() > 2 * 1024 * 1024) {
            fail(QStringLiteral("The translation response was too large to display safely."));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            fail(QStringLiteral("The translation service returned an unreadable response."));
            return;
        }
        const QJsonObject root = document.object();
        const QJsonObject responseData = root.value(QStringLiteral("responseData")).toObject();
        const QString translatedText = responseData.value(QStringLiteral("translatedText")).toString().trimmed();
        if (translatedText.isEmpty()) {
            fail(QStringLiteral("The translation service did not return a translation."));
            return;
        }
        if (!recordTranslationLookup(normalizedText,
                                     normalizedSource,
                                     normalizedTarget,
                                     provider.id())) {
            fail(QStringLiteral("The translation result could not be saved locally."));
            return;
        }
        busy_ = false;
        resultTitle_ = QStringLiteral("%1 → %2").arg(normalizedSource, normalizedTarget);
        resultBody_ = translatedText;
        resultPronunciation_.clear();
        resultSource_ = QStringLiteral("MyMemory Translation API");
        lastError_.clear();
        emit stateChanged();
    });
    return true;
}

} // namespace Lumina::Application

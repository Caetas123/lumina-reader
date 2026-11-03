#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QString>
#include <QVariantList>
#include <QUrl>

class QNetworkReply;

namespace Lumina::Infrastructure {
class Database;
}

namespace Lumina::Application {

class LookupController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString resultTitle READ resultTitle NOTIFY stateChanged)
    Q_PROPERTY(QString resultBody READ resultBody NOTIFY stateChanged)
    Q_PROPERTY(QString resultPronunciation READ resultPronunciation NOTIFY stateChanged)
    Q_PROPERTY(QString resultSource READ resultSource NOTIFY stateChanged)
    Q_PROPERTY(bool resultDictionaryHistoryAvailable READ resultDictionaryHistoryAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool resultDictionaryFavorite READ resultDictionaryFavorite NOTIFY stateChanged)
    Q_PROPERTY(bool resultTranslationHistoryAvailable READ resultTranslationHistoryAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool resultTranslationFavorite READ resultTranslationFavorite NOTIFY stateChanged)
    Q_PROPERTY(QVariantList dictionaryHistory READ dictionaryHistory NOTIFY stateChanged)
    Q_PROPERTY(QVariantList translationHistory READ translationHistory NOTIFY stateChanged)

public:
    explicit LookupController(QObject *parent = nullptr);

    [[nodiscard]] QString lastError() const;
    [[nodiscard]] bool busy() const;
    [[nodiscard]] QString resultTitle() const;
    [[nodiscard]] QString resultBody() const;
    [[nodiscard]] QString resultPronunciation() const;
    [[nodiscard]] QString resultSource() const;
    [[nodiscard]] bool resultDictionaryHistoryAvailable() const;
    [[nodiscard]] bool resultDictionaryFavorite() const;
    [[nodiscard]] bool resultTranslationHistoryAvailable() const;
    [[nodiscard]] bool resultTranslationFavorite() const;
    [[nodiscard]] QVariantList dictionaryHistory() const;
    [[nodiscard]] QVariantList translationHistory() const;
    void setDatabase(Lumina::Infrastructure::Database *database);

    Q_INVOKABLE bool openGoogle(const QString &text);
    Q_INVOKABLE bool openDictionary(const QString &term, const QString &language = {});
    Q_INVOKABLE bool openTranslation(const QString &text,
                                     const QString &sourceLanguage = {},
                                     const QString &targetLanguage = QStringLiteral("en"));
    Q_INVOKABLE bool lookupDictionary(const QString &term, const QString &language = {});
    Q_INVOKABLE bool translateText(const QString &text,
                                   const QString &sourceLanguage = {},
                                   const QString &targetLanguage = QStringLiteral("en"));
    Q_INVOKABLE bool setDictionaryFavorite(qint64 historyId, bool favorite);
    Q_INVOKABLE bool setTranslationFavorite(qint64 historyId, bool favorite);
    Q_INVOKABLE bool setCurrentDictionaryFavorite(bool favorite);
    Q_INVOKABLE bool setCurrentTranslationFavorite(bool favorite);
    Q_INVOKABLE bool clearDictionaryHistory();
    Q_INVOKABLE bool clearTranslationHistory();

signals:
    void stateChanged();

private:
    bool openUrl(const QUrl &url);
    bool recordDictionaryLookup(const QString &term,
                                const QString &language,
                                const QString &provider,
                                const QString &resultJson = QStringLiteral("{}"),
                                qint64 *historyId = nullptr);
    bool recordTranslationLookup(const QString &text,
                                 const QString &sourceLanguage,
                                 const QString &targetLanguage,
                                 const QString &provider,
                                 const QString &translatedText = QString(),
                                 qint64 *historyId = nullptr);
    bool setHistoryFavorite(const QString &tableName, qint64 historyId, bool favorite);
    void clearResult();
    void fail(const QString &message);
    bool loadCachedDictionary(const QString &term, const QString &language);
    bool loadCachedTranslation(const QString &text,
                               const QString &sourceLanguage,
                               const QString &targetLanguage);

    Lumina::Infrastructure::Database *database_ = nullptr;
    QNetworkAccessManager networkManager_;
    QPointer<QNetworkReply> activeReply_;
    bool busy_ = false;
    QString resultTitle_;
    QString resultBody_;
    QString resultPronunciation_;
    QString resultSource_;
    QString lastError_;
    qint64 resultDictionaryHistoryId_ = 0;
    bool resultDictionaryFavorite_ = false;
    qint64 resultTranslationHistoryId_ = 0;
    bool resultTranslationFavorite_ = false;
};

} // namespace Lumina::Application

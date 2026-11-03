#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QString>
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

public:
    explicit LookupController(QObject *parent = nullptr);

    [[nodiscard]] QString lastError() const;
    [[nodiscard]] bool busy() const;
    [[nodiscard]] QString resultTitle() const;
    [[nodiscard]] QString resultBody() const;
    [[nodiscard]] QString resultPronunciation() const;
    [[nodiscard]] QString resultSource() const;
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

signals:
    void stateChanged();

private:
    bool openUrl(const QUrl &url);
    bool recordDictionaryLookup(const QString &term,
                                const QString &language,
                                const QString &provider,
                                const QString &resultJson = QStringLiteral("{}"));
    bool recordTranslationLookup(const QString &text,
                                 const QString &sourceLanguage,
                                 const QString &targetLanguage,
                                 const QString &provider);
    void clearResult();
    void fail(const QString &message);

    Lumina::Infrastructure::Database *database_ = nullptr;
    QNetworkAccessManager networkManager_;
    QPointer<QNetworkReply> activeReply_;
    bool busy_ = false;
    QString resultTitle_;
    QString resultBody_;
    QString resultPronunciation_;
    QString resultSource_;
    QString lastError_;
};

} // namespace Lumina::Application

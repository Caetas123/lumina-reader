#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

namespace Lumina::Infrastructure {
class Database;
}

namespace Lumina::Application {

class LookupController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY stateChanged)

public:
    explicit LookupController(QObject *parent = nullptr);

    [[nodiscard]] QString lastError() const;
    void setDatabase(Lumina::Infrastructure::Database *database);

    Q_INVOKABLE bool openGoogle(const QString &text);
    Q_INVOKABLE bool openDictionary(const QString &term, const QString &language = {});
    Q_INVOKABLE bool openTranslation(const QString &text,
                                     const QString &sourceLanguage = {},
                                     const QString &targetLanguage = QStringLiteral("en"));

signals:
    void stateChanged();

private:
    bool openUrl(const QUrl &url);
    bool recordDictionaryLookup(const QString &term, const QString &language);
    bool recordTranslationLookup(const QString &text,
                                 const QString &sourceLanguage,
                                 const QString &targetLanguage);
    void fail(const QString &message);

    Lumina::Infrastructure::Database *database_ = nullptr;
    QString lastError_;
};

} // namespace Lumina::Application

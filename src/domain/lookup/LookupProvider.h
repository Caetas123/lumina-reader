#pragma once

#include <QUrl>
#include <QString>

namespace Lumina::Domain {

class DictionaryProvider {
public:
    virtual ~DictionaryProvider() = default;
    [[nodiscard]] virtual QString id() const = 0;
    [[nodiscard]] virtual QUrl lookupUrl(const QString &term, const QString &language) const = 0;
};

class TranslationProvider {
public:
    virtual ~TranslationProvider() = default;
    [[nodiscard]] virtual QString id() const = 0;
    [[nodiscard]] virtual QUrl translationUrl(const QString &text,
                                              const QString &sourceLanguage,
                                              const QString &targetLanguage) const = 0;
};

} // namespace Lumina::Domain

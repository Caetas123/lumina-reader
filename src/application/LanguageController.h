#pragma once

#include <QtCore/qobject.h>
#include <QtCore/qlocale.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qtranslator.h>

class QQmlEngine;

namespace Lumina::Application {

class SettingsController;

class LuminaTranslator final : public QTranslator {
public:
    void setLocale(const QString &locale) { locale_ = locale; }

    QString translate(const char *context,
                      const char *sourceText,
                      const char *disambiguation = nullptr,
                      int n = -1) const override;
    bool isEmpty() const override;

private:
    QString locale_;
};

// Owns the application UI language, including system-locale detection and the
// persisted manual override. The locale list intentionally follows the
// Windows desktop language-pack/LIP catalog so the same selector is available
// on Windows and Linux.
class LanguageController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString selectedLocale READ selectedLocale NOTIFY languageChanged)
    Q_PROPERTY(QString locale READ locale NOTIFY languageChanged)
    Q_PROPERTY(QString systemLocale READ systemLocale CONSTANT)
    Q_PROPERTY(QString systemLanguageLabel READ systemLanguageLabel CONSTANT)
    Q_PROPERTY(QStringList availableLocales READ availableLocales CONSTANT)
    Q_PROPERTY(QStringList availableLanguageLabels READ availableLanguageLabels CONSTANT)
    Q_PROPERTY(QStringList availableSelectionLocales READ availableSelectionLocales CONSTANT)
    Q_PROPERTY(bool usesSystemLanguage READ usesSystemLanguage NOTIFY languageChanged)
    Q_PROPERTY(bool translationAvailable READ translationAvailable NOTIFY languageChanged)
    Q_PROPERTY(QString translationStatus READ translationStatus NOTIFY languageChanged)
    Q_PROPERTY(bool rightToLeft READ rightToLeft NOTIFY languageChanged)

public:
    explicit LanguageController(SettingsController *settings, QObject *parent = nullptr);
    ~LanguageController() override;

    [[nodiscard]] QString selectedLocale() const;
    [[nodiscard]] QString locale() const;
    [[nodiscard]] QString systemLocale() const;
    [[nodiscard]] QString systemLanguageLabel() const;
    [[nodiscard]] QStringList availableLocales() const;
    [[nodiscard]] QStringList availableLanguageLabels() const;
    [[nodiscard]] QStringList availableSelectionLocales() const;
    [[nodiscard]] bool usesSystemLanguage() const;
    [[nodiscard]] bool translationAvailable() const;
    [[nodiscard]] QString translationStatus() const;
    [[nodiscard]] bool rightToLeft() const;

    void setQmlEngine(QQmlEngine *engine);

    Q_INVOKABLE bool setLanguage(const QString &locale);
    Q_INVOKABLE QString labelForLocale(const QString &locale) const;
    Q_INVOKABLE QString displayLocaleFor(const QString &locale) const;
    Q_INVOKABLE QString formatInteger(double value) const;
    Q_INVOKABLE QString formatDecimal(double value, int fractionDigits = 2) const;
    Q_INVOKABLE QString formatPercent(double value, int fractionDigits = 0) const;
    Q_INVOKABLE int parseInteger(const QString &value) const;

signals:
    void languageChanged();

private:
    void applyLanguage();
    [[nodiscard]] QString effectiveLocaleFor(const QString &selection) const;
    [[nodiscard]] QString normalizedLocale(const QString &locale) const;
    [[nodiscard]] QStringList uiLanguageCandidates() const;
    [[nodiscard]] QLocale formattingLocale() const;

    SettingsController *settings_ = nullptr;
    QQmlEngine *qmlEngine_ = nullptr;
    LuminaTranslator translator_;
    QString selectedLocale_ = QStringLiteral("system");
    QString effectiveLocale_ = QStringLiteral("en-US");
    QString systemLocale_ = QStringLiteral("en-US");
    QString systemLanguageLabel_ = QStringLiteral("English (United States)");
    bool translatorInstalled_ = false;
};

} // namespace Lumina::Application

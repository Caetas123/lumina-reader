#pragma once

#include <QObject>
#include <QString>

namespace Lumina::Infrastructure {
class Database;
}

namespace Lumina::Application {

class SettingsController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY settingsChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY settingsChanged)
    Q_PROPERTY(int lineSpacing READ lineSpacing WRITE setLineSpacing NOTIFY settingsChanged)
    Q_PROPERTY(bool continuousReading READ continuousReading WRITE setContinuousReading NOTIFY settingsChanged)
    Q_PROPERTY(bool highContrast READ highContrast WRITE setHighContrast NOTIFY settingsChanged)

public:
    explicit SettingsController(QObject *parent = nullptr);

    [[nodiscard]] QString theme() const;
    [[nodiscard]] int fontSize() const;
    [[nodiscard]] int lineSpacing() const;
    [[nodiscard]] bool continuousReading() const;
    [[nodiscard]] bool highContrast() const;
    [[nodiscard]] QString fontFamily() const;
    [[nodiscard]] int readerWidth() const;
    [[nodiscard]] int readerMargin() const;
    [[nodiscard]] QString textAlignment() const;
    [[nodiscard]] QString imageFit() const;
    [[nodiscard]] QString readingDirection() const;
    [[nodiscard]] QString customReaderBackground() const;
    [[nodiscard]] QString customReaderText() const;
    [[nodiscard]] QString customHighlightColor() const;

    void setDatabase(Lumina::Infrastructure::Database *database);
    Q_INVOKABLE void reload();

public slots:
    void setTheme(const QString &theme);
    void setFontSize(int fontSize);
    void setLineSpacing(int lineSpacing);
    void setContinuousReading(bool enabled);
    void setHighContrast(bool enabled);
    void setFontFamily(const QString &fontFamily);
    void setReaderWidth(int width);
    void setReaderMargin(int margin);
    void setTextAlignment(const QString &alignment);
    void setImageFit(const QString &fit);
    void setReadingDirection(const QString &direction);
    void setCustomReaderBackground(const QString &color);
    void setCustomReaderText(const QString &color);
    void setCustomHighlightColor(const QString &color);

    [[nodiscard]] QString readerPreferenceValue(const QString &key,
                                                const QString &format,
                                                const QString &contentType,
                                                const QString &documentId,
                                                const QString &fallback) const;
    void setReaderPreferenceValue(const QString &key,
                                  const QString &scope,
                                  const QString &format,
                                  const QString &contentType,
                                  const QString &documentId,
                                  const QString &value);

signals:
    void settingsChanged();

private:
    void load();
    void persist(const QString &key, const QString &value);

    Lumina::Infrastructure::Database *database_ = nullptr;
    QString theme_ = QStringLiteral("light");
    int fontSize_ = 18;
    int lineSpacing_ = 120;
    bool continuousReading_ = true;
    bool highContrast_ = false;
    QString fontFamily_ = QStringLiteral("Segoe UI");
    int readerWidth_ = 860;
    int readerMargin_ = 30;
    QString textAlignment_ = QStringLiteral("left");
    QString imageFit_ = QStringLiteral("page");
    QString readingDirection_ = QStringLiteral("ltr");
    QString customReaderBackground_;
    QString customReaderText_;
    QString customHighlightColor_;
};

} // namespace Lumina::Application

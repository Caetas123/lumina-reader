#pragma once

#include <QObject>
#include <QString>

namespace Lumina::Application {
class SettingsController;

class ReaderPreferencesController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY preferencesChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY preferencesChanged)
    Q_PROPERTY(int lineSpacing READ lineSpacing WRITE setLineSpacing NOTIFY preferencesChanged)
    Q_PROPERTY(bool continuousReading READ continuousReading WRITE setContinuousReading NOTIFY preferencesChanged)
    Q_PROPERTY(bool highContrast READ highContrast WRITE setHighContrast NOTIFY preferencesChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY preferencesChanged)
    Q_PROPERTY(int readerWidth READ readerWidth WRITE setReaderWidth NOTIFY preferencesChanged)
    Q_PROPERTY(int readerMargin READ readerMargin WRITE setReaderMargin NOTIFY preferencesChanged)
    Q_PROPERTY(QString textAlignment READ textAlignment WRITE setTextAlignment NOTIFY preferencesChanged)
    Q_PROPERTY(QString imageFit READ imageFit WRITE setImageFit NOTIFY preferencesChanged)
    Q_PROPERTY(QString readingDirection READ readingDirection WRITE setReadingDirection NOTIFY preferencesChanged)
    Q_PROPERTY(QString customReaderBackground READ customReaderBackground WRITE setCustomReaderBackground NOTIFY preferencesChanged)
    Q_PROPERTY(QString customReaderText READ customReaderText WRITE setCustomReaderText NOTIFY preferencesChanged)
    Q_PROPERTY(QString customHighlightColor READ customHighlightColor WRITE setCustomHighlightColor NOTIFY preferencesChanged)
    Q_PROPERTY(QString scope READ scope WRITE setScope NOTIFY preferencesChanged)
    Q_PROPERTY(bool contextAvailable READ contextAvailable NOTIFY preferencesChanged)

public:
    explicit ReaderPreferencesController(SettingsController *settings, QObject *parent = nullptr);

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
    [[nodiscard]] QString scope() const;
    [[nodiscard]] bool contextAvailable() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setContext(const QString &format,
                                const QString &contentType,
                                const QString &documentId);

public slots:
    void setTheme(const QString &value);
    void setFontSize(int value);
    void setLineSpacing(int value);
    void setContinuousReading(bool value);
    void setHighContrast(bool value);
    void setFontFamily(const QString &value);
    void setReaderWidth(int value);
    void setReaderMargin(int value);
    void setTextAlignment(const QString &value);
    void setImageFit(const QString &value);
    void setReadingDirection(const QString &value);
    void setCustomReaderBackground(const QString &value);
    void setCustomReaderText(const QString &value);
    void setCustomHighlightColor(const QString &value);
    void setScope(const QString &value);

signals:
    void preferencesChanged();

private:
    [[nodiscard]] QString value(const QString &key, const QString &fallback) const;
    void setValue(const QString &key, const QString &value);

    SettingsController *settings_ = nullptr;
    QString format_;
    QString contentType_;
    QString documentId_;
    QString scope_ = QStringLiteral("global");
};

} // namespace Lumina::Application

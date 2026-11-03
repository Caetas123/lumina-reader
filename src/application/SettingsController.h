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

    void setDatabase(Lumina::Infrastructure::Database *database);

public slots:
    void setTheme(const QString &theme);
    void setFontSize(int fontSize);
    void setLineSpacing(int lineSpacing);
    void setContinuousReading(bool enabled);
    void setHighContrast(bool enabled);

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
};

} // namespace Lumina::Application

#include "application/SettingsController.h"

#include "infrastructure/database/Database.h"

#include <QSqlQuery>
#include <QStringList>

#include <algorithm>

namespace {

bool isThemeSupported(const QString &theme)
{
    static const QStringList themes = {
        QStringLiteral("light"), QStringLiteral("dark"), QStringLiteral("sepia"),
        QStringLiteral("paper"), QStringLiteral("gray"), QStringLiteral("high-contrast"),
        QStringLiteral("low-light"),
    };
    return themes.contains(theme);
}

} // namespace

namespace Lumina::Application {

SettingsController::SettingsController(QObject *parent)
    : QObject(parent)
{
}

QString SettingsController::theme() const
{
    return theme_;
}

int SettingsController::fontSize() const
{
    return fontSize_;
}

int SettingsController::lineSpacing() const
{
    return lineSpacing_;
}

bool SettingsController::continuousReading() const
{
    return continuousReading_;
}

bool SettingsController::highContrast() const
{
    return highContrast_;
}

void SettingsController::setDatabase(Lumina::Infrastructure::Database *database)
{
    database_ = database;
    load();
}

void SettingsController::reload()
{
    load();
}

void SettingsController::setTheme(const QString &theme)
{
    const QString normalized = theme.trimmed().toLower();
    if (!isThemeSupported(normalized) || normalized == theme_) {
        return;
    }
    theme_ = normalized;
    persist(QStringLiteral("theme"), theme_);
    emit settingsChanged();
}

void SettingsController::setFontSize(int fontSize)
{
    const int normalized = std::clamp(fontSize, 12, 32);
    if (normalized == fontSize_) {
        return;
    }
    fontSize_ = normalized;
    persist(QStringLiteral("font_size"), QString::number(fontSize_));
    emit settingsChanged();
}

void SettingsController::setLineSpacing(int lineSpacing)
{
    const int normalized = std::clamp(lineSpacing, 100, 220);
    if (normalized == lineSpacing_) {
        return;
    }
    lineSpacing_ = normalized;
    persist(QStringLiteral("line_spacing"), QString::number(lineSpacing_));
    emit settingsChanged();
}

void SettingsController::setContinuousReading(bool enabled)
{
    if (enabled == continuousReading_) {
        return;
    }
    continuousReading_ = enabled;
    persist(QStringLiteral("continuous_reading"), enabled ? QStringLiteral("1") : QStringLiteral("0"));
    emit settingsChanged();
}

void SettingsController::setHighContrast(bool enabled)
{
    if (enabled == highContrast_) {
        return;
    }
    highContrast_ = enabled;
    persist(QStringLiteral("high_contrast"), enabled ? QStringLiteral("1") : QStringLiteral("0"));
    emit settingsChanged();
}

void SettingsController::load()
{
    if (database_ == nullptr || !database_->isOpen()) {
        return;
    }

    QSqlQuery query(database_->connection());
    if (!query.exec(QStringLiteral("SELECT key, value FROM app_settings"))) {
        return;
    }
    while (query.next()) {
        const QString key = query.value(0).toString();
        const QString value = query.value(1).toString();
        if (key == QStringLiteral("theme") && isThemeSupported(value)) {
            theme_ = value;
        } else if (key == QStringLiteral("font_size")) {
            fontSize_ = std::clamp(value.toInt(), 12, 32);
        } else if (key == QStringLiteral("line_spacing")) {
            lineSpacing_ = std::clamp(value.toInt(), 100, 220);
        } else if (key == QStringLiteral("continuous_reading")) {
            continuousReading_ = value == QStringLiteral("1");
        } else if (key == QStringLiteral("high_contrast")) {
            highContrast_ = value == QStringLiteral("1");
        }
    }
    emit settingsChanged();
}

void SettingsController::persist(const QString &key, const QString &value)
{
    if (database_ == nullptr || !database_->isOpen()) {
        return;
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "INSERT INTO app_settings (key, value, updated_at) VALUES (:key, :value, CURRENT_TIMESTAMP) "
        "ON CONFLICT(key) DO UPDATE SET value = excluded.value, updated_at = CURRENT_TIMESTAMP"));
    query.bindValue(QStringLiteral(":key"), key);
    query.bindValue(QStringLiteral(":value"), value);
    query.exec();
}

} // namespace Lumina::Application

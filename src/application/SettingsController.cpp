#include "application/SettingsController.h"

#include "infrastructure/database/Database.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>

#include <algorithm>

namespace {

constexpr QLatin1StringView defaultProfileId("default");
constexpr QLatin1StringView defaultProfileName("Default");

bool isThemeSupported(const QString &theme)
{
    static const QStringList themes = {
        QStringLiteral("light"), QStringLiteral("dark"), QStringLiteral("sepia"),
        QStringLiteral("paper"), QStringLiteral("gray"), QStringLiteral("high-contrast"),
        QStringLiteral("low-light"),
    };
    return themes.contains(theme);
}

bool isHexColor(const QString &value)
{
    static const QRegularExpression pattern(QStringLiteral("^#[0-9a-fA-F]{6,8}$"));
    return pattern.match(value).hasMatch();
}

bool isAlignmentSupported(const QString &alignment)
{
    return alignment == QStringLiteral("left") || alignment == QStringLiteral("center")
        || alignment == QStringLiteral("right") || alignment == QStringLiteral("justify");
}

bool isImageFitSupported(const QString &fit)
{
    return fit == QStringLiteral("page") || fit == QStringLiteral("width")
        || fit == QStringLiteral("height") || fit == QStringLiteral("actual");
}

bool isReadingDirectionSupported(const QString &direction)
{
    return direction == QStringLiteral("ltr") || direction == QStringLiteral("rtl");
}

QString normalizedContextToken(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    static const QRegularExpression allowed(QStringLiteral("^[a-z0-9_-]{1,128}$"));
    return allowed.match(normalized).hasMatch() ? normalized : QString();
}

QString profileKey(const QString &profileId, const QString &key)
{
    return QStringLiteral("profile.%1.%2").arg(profileId, key);
}

QString scopedReaderKey(const QString &key,
                        const QString &scope,
                        const QString &format,
                        const QString &contentType,
                        const QString &documentId)
{
    const QString normalizedKey = key.trimmed().toLower();
    const QString normalizedScope = scope.trimmed().toLower();
    if (normalizedScope == QStringLiteral("format")) {
        const QString token = normalizedContextToken(format);
        if (!token.isEmpty()) {
            return QStringLiteral("reader.format.%1.%2").arg(token, normalizedKey);
        }
    } else if (normalizedScope == QStringLiteral("type")) {
        const QString token = normalizedContextToken(contentType);
        if (!token.isEmpty()) {
            return QStringLiteral("reader.type.%1.%2").arg(token, normalizedKey);
        }
    } else if (normalizedScope == QStringLiteral("document")) {
        const QString token = normalizedContextToken(documentId);
        if (!token.isEmpty()) {
            return QStringLiteral("reader.document.%1.%2").arg(token, normalizedKey);
        }
    }
    return normalizedKey;
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

QString SettingsController::languageLocale() const
{
    return languageLocale_;
}

QString SettingsController::activeProfile() const
{
    return profileNameForId(activeProfileId_);
}

QStringList SettingsController::profileNames() const
{
    QStringList names;
    names.reserve(profileIds_.size() + 1);
    names.append(QString::fromLatin1(defaultProfileName));
    for (const QString &id : profileIds_) {
        names.append(profileNames_.value(id, id));
    }
    return names;
}

QString SettingsController::fontFamily() const
{
    return fontFamily_;
}

int SettingsController::readerWidth() const
{
    return readerWidth_;
}


int SettingsController::readerMargin() const
{
    return readerMargin_;
}

QString SettingsController::textAlignment() const
{
    return textAlignment_;
}

QString SettingsController::imageFit() const
{
    return imageFit_;
}

QString SettingsController::readingDirection() const
{
    return readingDirection_;
}

QString SettingsController::customReaderBackground() const
{
    return customReaderBackground_;
}

QString SettingsController::customReaderText() const
{
    return customReaderText_;
}

QString SettingsController::customHighlightColor() const
{
    return customHighlightColor_;
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

void SettingsController::setActiveProfile(const QString &profile)
{
    if (database_ == nullptr || !database_->isOpen()) {
        return;
    }
    const QString requested = profile.trimmed();
    QString nextId;
    if (requested.compare(QString::fromLatin1(defaultProfileId), Qt::CaseInsensitive) == 0
        || requested.compare(QString::fromLatin1(defaultProfileName), Qt::CaseInsensitive) == 0) {
        nextId = QString::fromLatin1(defaultProfileId);
    } else {
        for (const QString &id : profileIds_) {
            if (id.compare(requested, Qt::CaseInsensitive) == 0
                || profileNames_.value(id).compare(requested, Qt::CaseInsensitive) == 0) {
                nextId = id;
                break;
            }
        }
    }
    if (nextId.isEmpty() || nextId == activeProfileId_) {
        return;
    }

    activeProfileId_ = nextId;
    if (activeProfileId_ == QString::fromLatin1(defaultProfileId)) {
        QSqlQuery query(database_->connection());
        query.exec(QStringLiteral("DELETE FROM app_settings WHERE key = 'active_profile'"));
    } else {
        QSqlQuery query(database_->connection());
        query.prepare(QStringLiteral(
            "INSERT INTO app_settings (key, value, updated_at) VALUES ('active_profile', :value, CURRENT_TIMESTAMP) "
            "ON CONFLICT(key) DO UPDATE SET value = excluded.value, updated_at = CURRENT_TIMESTAMP"));
        query.bindValue(QStringLiteral(":value"), activeProfileId_);
        query.exec();
    }
    load();
    emit profilesChanged();
}

bool SettingsController::createProfile(const QString &name)
{
    if (database_ == nullptr || !database_->isOpen()) {
        return false;
    }
    const QString displayName = name.trimmed().left(64);
    if (displayName.isEmpty()
        || displayName.compare(QString::fromLatin1(defaultProfileName), Qt::CaseInsensitive) == 0) {
        return false;
    }
    for (const QString &id : profileIds_) {
        if (profileNames_.value(id).compare(displayName, Qt::CaseInsensitive) == 0) {
            return false;
        }
    }

    QString id = profileIdForName(displayName);
    if (id.isEmpty()) {
        return false;
    }
    const QString baseId = id;
    int suffix = 2;
    while (profileIds_.contains(id)) {
        id = QStringLiteral("%1-%2").arg(baseId).arg(suffix++);
    }

    const QString sourcePrefix = profileStoragePrefix();
    const QString targetPrefix = profileKey(id, QString());
    QSqlQuery source(database_->connection());
    if (!source.exec(QStringLiteral("SELECT key, value FROM app_settings"))) {
        return false;
    }
    while (source.next()) {
        const QString sourceKey = source.value(0).toString();
        if (sourceKey == QStringLiteral("profiles") || sourceKey == QStringLiteral("active_profile")
            || sourceKey == QStringLiteral("language_locale")) {
            continue;
        }
        QString baseKey;
        if (sourcePrefix.isEmpty()) {
            if (sourceKey.startsWith(QStringLiteral("profile."))) {
                continue;
            }
            baseKey = sourceKey;
        } else if (sourceKey.startsWith(sourcePrefix)) {
            baseKey = sourceKey.mid(sourcePrefix.size());
        } else {
            continue;
        }
        if (baseKey.isEmpty()) {
            continue;
        }
        QSqlQuery copy(database_->connection());
        copy.prepare(QStringLiteral(
            "INSERT INTO app_settings (key, value, updated_at) VALUES (:key, :value, CURRENT_TIMESTAMP) "
            "ON CONFLICT(key) DO UPDATE SET value = excluded.value, updated_at = CURRENT_TIMESTAMP"));
        copy.bindValue(QStringLiteral(":key"), targetPrefix + baseKey);
        copy.bindValue(QStringLiteral(":value"), source.value(1).toString());
        if (!copy.exec()) {
            return false;
        }
    }

    profileIds_.append(id);
    profileNames_.insert(id, displayName);
    persistProfiles(profileIds_, profileNames_);
    emit profilesChanged();
    setActiveProfile(displayName);
    return true;
}

bool SettingsController::deleteProfile(const QString &name)
{
    if (database_ == nullptr || !database_->isOpen()) {
        return false;
    }
    QString id;
    const QString requested = name.trimmed();
    for (const QString &candidate : profileIds_) {
        if (candidate.compare(requested, Qt::CaseInsensitive) == 0
            || profileNames_.value(candidate).compare(requested, Qt::CaseInsensitive) == 0) {
            id = candidate;
            break;
        }
    }
    if (id.isEmpty()) {
        return false;
    }

    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral("DELETE FROM app_settings WHERE key LIKE :prefix"));
    query.bindValue(QStringLiteral(":prefix"), profileKey(id, QStringLiteral("%")));
    if (!query.exec()) {
        return false;
    }
    profileIds_.removeAll(id);
    profileNames_.remove(id);
    persistProfiles(profileIds_, profileNames_);
    const bool wasActive = activeProfileId_ == id;
    if (wasActive) {
        activeProfileId_.clear();
        setActiveProfile(QString::fromLatin1(defaultProfileName));
    } else {
        emit profilesChanged();
    }
    return true;
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

void SettingsController::setLanguageLocale(const QString &locale)
{
    QString normalized = locale.trimmed().replace(QLatin1Char('_'), QLatin1Char('-'));
    if (normalized.isEmpty()) {
        normalized = QStringLiteral("system");
    }
    if (normalized.compare(QStringLiteral("system"), Qt::CaseInsensitive) == 0) {
        normalized = QStringLiteral("system");
    }
    if (normalized == languageLocale_) {
        return;
    }
    languageLocale_ = normalized;
    persistGlobal(QStringLiteral("language_locale"), languageLocale_);
    emit settingsChanged();
}

void SettingsController::setFontFamily(const QString &fontFamily)
{
    const QString normalized = fontFamily.trimmed().left(64);
    if (normalized.isEmpty() || normalized == fontFamily_) {
        return;
    }
    fontFamily_ = normalized;
    persist(QStringLiteral("font_family"), fontFamily_);
    emit settingsChanged();
}

void SettingsController::setReaderWidth(int width)
{
    const int normalized = std::clamp(width, 600, 1400);
    if (normalized == readerWidth_) {
        return;
    }
    readerWidth_ = normalized;
    persist(QStringLiteral("reader_width"), QString::number(readerWidth_));
    emit settingsChanged();
}


void SettingsController::setReaderMargin(int margin)
{
    const int normalized = std::clamp(margin, 12, 80);
    if (normalized == readerMargin_) {
        return;
    }
    readerMargin_ = normalized;
    persist(QStringLiteral("reader_margin"), QString::number(readerMargin_));
    emit settingsChanged();
}

void SettingsController::setTextAlignment(const QString &alignment)
{
    const QString normalized = alignment.trimmed().toLower();
    if (!isAlignmentSupported(normalized) || normalized == textAlignment_) {
        return;
    }
    textAlignment_ = normalized;
    persist(QStringLiteral("text_alignment"), textAlignment_);
    emit settingsChanged();
}

void SettingsController::setImageFit(const QString &fit)
{
    const QString normalized = fit.trimmed().toLower();
    if (!isImageFitSupported(normalized) || normalized == imageFit_) {
        return;
    }
    imageFit_ = normalized;
    persist(QStringLiteral("image_fit"), imageFit_);
    emit settingsChanged();
}

void SettingsController::setReadingDirection(const QString &direction)
{
    const QString normalized = direction.trimmed().toLower();
    if (!isReadingDirectionSupported(normalized) || normalized == readingDirection_) {
        return;
    }
    readingDirection_ = normalized;
    persist(QStringLiteral("reading_direction"), readingDirection_);
    emit settingsChanged();
}

void SettingsController::setCustomReaderBackground(const QString &color)
{
    const QString normalized = color.trimmed().toLower();
    if ((!normalized.isEmpty() && !isHexColor(normalized)) || normalized == customReaderBackground_) {
        return;
    }
    customReaderBackground_ = normalized;
    persist(QStringLiteral("custom_reader_background"), customReaderBackground_);
    emit settingsChanged();
}

void SettingsController::setCustomReaderText(const QString &color)
{
    const QString normalized = color.trimmed().toLower();
    if ((!normalized.isEmpty() && !isHexColor(normalized)) || normalized == customReaderText_) {
        return;
    }
    customReaderText_ = normalized;
    persist(QStringLiteral("custom_reader_text"), customReaderText_);
    emit settingsChanged();
}

void SettingsController::setCustomHighlightColor(const QString &color)
{
    const QString normalized = color.trimmed().toLower();
    if ((!normalized.isEmpty() && !isHexColor(normalized)) || normalized == customHighlightColor_) {
        return;
    }
    customHighlightColor_ = normalized;
    persist(QStringLiteral("custom_highlight_color"), customHighlightColor_);
    emit settingsChanged();
}

QString SettingsController::readerPreferenceValue(const QString &key,
                                                  const QString &format,
                                                  const QString &contentType,
                                                  const QString &documentId,
                                                  const QString &fallback) const
{
    if (database_ == nullptr || !database_->isOpen()) {
        return fallback;
    }

    QStringList keys;
    const QString normalizedKey = key.trimmed().toLower();
    const QString documentToken = normalizedContextToken(documentId);
    const QString typeToken = normalizedContextToken(contentType);
    const QString formatToken = normalizedContextToken(format);
    if (!documentToken.isEmpty()) {
        keys.append(QStringLiteral("reader.document.%1.%2").arg(documentToken, normalizedKey));
    }
    if (!typeToken.isEmpty()) {
        keys.append(QStringLiteral("reader.type.%1.%2").arg(typeToken, normalizedKey));
    }
    if (!formatToken.isEmpty()) {
        keys.append(QStringLiteral("reader.format.%1.%2").arg(formatToken, normalizedKey));
    }
    keys.append(normalizedKey);

    const QString prefix = profileStoragePrefix();
    for (const QString &candidate : keys) {
        QSqlQuery query(database_->connection());
        query.prepare(QStringLiteral("SELECT value FROM app_settings WHERE key = :key"));
        query.bindValue(QStringLiteral(":key"), prefix + candidate);
        if (query.exec() && query.next()) {
            return query.value(0).toString();
        }
    }
    return fallback;
}

void SettingsController::setReaderPreferenceValue(const QString &key,
                                                  const QString &scope,
                                                  const QString &format,
                                                  const QString &contentType,
                                                  const QString &documentId,
                                                  const QString &value)
{
    const QString normalizedKey = key.trimmed().toLower();
    if (normalizedKey.isEmpty()) {
        return;
    }
    const QString storageKey = scopedReaderKey(
        normalizedKey, scope, format, contentType, documentId);
    persist(storageKey, value);
    emit settingsChanged();
}

void SettingsController::load()
{
    if (database_ == nullptr || !database_->isOpen()) {
        return;
    }

    profileIds_ = storedProfileIds();
    QString storedActiveProfile;
    QSqlQuery activeQuery(database_->connection());
    activeQuery.prepare(QStringLiteral("SELECT value FROM app_settings WHERE key = 'active_profile'"));
    if (activeQuery.exec() && activeQuery.next()) {
        storedActiveProfile = activeQuery.value(0).toString();
    }
    if (profileIds_.contains(storedActiveProfile)) {
        activeProfileId_ = storedActiveProfile;
    } else {
        activeProfileId_ = QString::fromLatin1(defaultProfileId);
    }

    resetDefaults();
    QSqlQuery languageQuery(database_->connection());
    languageQuery.prepare(QStringLiteral("SELECT value FROM app_settings WHERE key = 'language_locale'"));
    if (languageQuery.exec() && languageQuery.next() && !languageQuery.value(0).toString().trimmed().isEmpty()) {
        languageLocale_ = languageQuery.value(0).toString().trimmed().replace(QLatin1Char('_'), QLatin1Char('-'));
    }
    const QString prefix = profileStoragePrefix();
    QSqlQuery query(database_->connection());
    if (!query.exec(QStringLiteral("SELECT key, value FROM app_settings"))) {
        return;
    }
    while (query.next()) {
        QString key = query.value(0).toString();
        if (prefix.isEmpty()) {
            if (key.startsWith(QStringLiteral("profile."))) {
                continue;
            }
        } else {
            if (!key.startsWith(prefix)) {
                continue;
            }
            key.remove(0, prefix.size());
        }
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
        } else if (key == QStringLiteral("language_locale") && !value.trimmed().isEmpty()) {
            languageLocale_ = value.trimmed().replace(QLatin1Char('_'), QLatin1Char('-'));
        } else if (key == QStringLiteral("font_family") && !value.trimmed().isEmpty()) {
            fontFamily_ = value.trimmed().left(64);
        } else if (key == QStringLiteral("reader_width")) {
            readerWidth_ = std::clamp(value.toInt(), 600, 1400);
        } else if (key == QStringLiteral("reader_margin")) {
            readerMargin_ = std::clamp(value.toInt(), 12, 80);
        } else if (key == QStringLiteral("text_alignment") && isAlignmentSupported(value)) {
            textAlignment_ = value;
        } else if (key == QStringLiteral("image_fit") && isImageFitSupported(value)) {
            imageFit_ = value;
        } else if (key == QStringLiteral("reading_direction") && isReadingDirectionSupported(value)) {
            readingDirection_ = value;
        } else if (key == QStringLiteral("custom_reader_background")
                   && (value.isEmpty() || isHexColor(value))) {
            customReaderBackground_ = value;
        } else if (key == QStringLiteral("custom_reader_text")
                   && (value.isEmpty() || isHexColor(value))) {
            customReaderText_ = value;
        } else if (key == QStringLiteral("custom_highlight_color")
                   && (value.isEmpty() || isHexColor(value))) {
            customHighlightColor_ = value;
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
    query.bindValue(QStringLiteral(":key"), profileStoragePrefix() + key);
    query.bindValue(QStringLiteral(":value"), value);
    query.exec();
}

void SettingsController::persistGlobal(const QString &key, const QString &value)
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

QString SettingsController::profileStoragePrefix() const
{
    return activeProfileId_ == QString::fromLatin1(defaultProfileId)
        ? QString()
        : profileKey(activeProfileId_, QString());
}

QString SettingsController::profileIdForName(const QString &name) const
{
    QString id = name.trimmed().toLower();
    id.replace(QRegularExpression(QStringLiteral("[^a-z0-9_-]+")), QStringLiteral("-"));
    id.replace(QRegularExpression(QStringLiteral("^-+|-+$")), QString());
    return id == QString::fromLatin1(defaultProfileId) ? QString() : id.left(48);
}

QString SettingsController::profileNameForId(const QString &id) const
{
    return id == QString::fromLatin1(defaultProfileId)
        ? QString::fromLatin1(defaultProfileName)
        : profileNames_.value(id, id);
}

QStringList SettingsController::storedProfileIds() const
{
    auto *self = const_cast<SettingsController *>(this);
    self->profileIds_.clear();
    self->profileNames_.clear();
    if (database_ == nullptr || !database_->isOpen()) {
        return {};
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral("SELECT value FROM app_settings WHERE key = 'profiles'"));
    if (!query.exec() || !query.next()) {
        return {};
    }
    const QJsonDocument document = QJsonDocument::fromJson(query.value(0).toByteArray());
    if (!document.isArray()) {
        return {};
    }
    for (const QJsonValue &value : document.array()) {
        const QJsonObject object = value.toObject();
        const QString id = object.value(QStringLiteral("id")).toString().trimmed().toLower();
        const QString name = object.value(QStringLiteral("name")).toString().trimmed().left(64);
        if (id.isEmpty() || id == QString::fromLatin1(defaultProfileId) || name.isEmpty()
            || self->profileIds_.contains(id)) {
            continue;
        }
        self->profileIds_.append(id);
        self->profileNames_.insert(id, name);
    }
    return self->profileIds_;
}

void SettingsController::persistProfiles(const QStringList &ids,
                                         const QHash<QString, QString> &names)
{
    if (database_ == nullptr || !database_->isOpen()) {
        return;
    }
    QJsonArray profiles;
    for (const QString &id : ids) {
        profiles.append(QJsonObject{{QStringLiteral("id"), id},
                                    {QStringLiteral("name"), names.value(id, id)}});
    }
    QSqlQuery query(database_->connection());
    query.prepare(QStringLiteral(
        "INSERT INTO app_settings (key, value, updated_at) VALUES ('profiles', :value, CURRENT_TIMESTAMP) "
        "ON CONFLICT(key) DO UPDATE SET value = excluded.value, updated_at = CURRENT_TIMESTAMP"));
    query.bindValue(QStringLiteral(":value"),
                    QString::fromUtf8(QJsonDocument(profiles).toJson(QJsonDocument::Compact)));
    query.exec();
}

void SettingsController::resetDefaults()
{
    theme_ = QStringLiteral("dark");
    fontSize_ = 18;
    lineSpacing_ = 120;
    continuousReading_ = true;
    highContrast_ = false;
    languageLocale_ = QStringLiteral("system");
    fontFamily_ = QStringLiteral("Segoe UI");
    readerWidth_ = 860;
    readerMargin_ = 30;
    textAlignment_ = QStringLiteral("left");
    imageFit_ = QStringLiteral("page");
    readingDirection_ = QStringLiteral("ltr");
    customReaderBackground_.clear();
    customReaderText_.clear();
    customHighlightColor_.clear();
}

} // namespace Lumina::Application

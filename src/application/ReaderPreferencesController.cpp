#include "application/ReaderPreferencesController.h"

#include "application/SettingsController.h"

#include <QRegularExpression>
#include <QStringList>

#include <algorithm>

namespace Lumina::Application {

namespace {

bool isHexColor(const QString &value)
{
    static const QRegularExpression pattern(QStringLiteral("^#[0-9a-fA-F]{6,8}$"));
    return value.isEmpty() || pattern.match(value).hasMatch();
}

} // namespace

ReaderPreferencesController::ReaderPreferencesController(SettingsController *settings, QObject *parent)
    : QObject(parent)
    , settings_(settings)
{
}

QString ReaderPreferencesController::theme() const
{
    return value(QStringLiteral("theme"),
                 settings_ != nullptr ? settings_->theme() : QStringLiteral("dark"));
}

int ReaderPreferencesController::fontSize() const
{
    return std::clamp(value(QStringLiteral("font_size"),
                            settings_ != nullptr ? QString::number(settings_->fontSize()) : QStringLiteral("18"))
                          .toInt(),
                      12,
                      32);
}

int ReaderPreferencesController::lineSpacing() const
{
    return std::clamp(value(QStringLiteral("line_spacing"),
                            settings_ != nullptr ? QString::number(settings_->lineSpacing()) : QStringLiteral("120"))
                          .toInt(),
                      100,
                      220);
}

bool ReaderPreferencesController::continuousReading() const
{
    return value(QStringLiteral("continuous_reading"),
                 settings_ != nullptr && settings_->continuousReading() ? QStringLiteral("1") : QStringLiteral("0"))
        == QStringLiteral("1");
}

bool ReaderPreferencesController::highContrast() const
{
    return value(QStringLiteral("high_contrast"),
                 settings_ != nullptr && settings_->highContrast() ? QStringLiteral("1") : QStringLiteral("0"))
        == QStringLiteral("1");
}

QString ReaderPreferencesController::fontFamily() const
{
    return value(QStringLiteral("font_family"),
                 settings_ != nullptr ? settings_->fontFamily() : QStringLiteral("Segoe UI"));
}

int ReaderPreferencesController::readerWidth() const
{
    return std::clamp(value(QStringLiteral("reader_width"),
                            settings_ != nullptr ? QString::number(settings_->readerWidth()) : QStringLiteral("860"))
                          .toInt(),
                      600,
                      1400);
}

int ReaderPreferencesController::readerMargin() const
{
    return std::clamp(value(QStringLiteral("reader_margin"),
                            settings_ != nullptr ? QString::number(settings_->readerMargin()) : QStringLiteral("30"))
                          .toInt(),
                      12,
                      80);
}

QString ReaderPreferencesController::textAlignment() const
{
    const QString result = value(QStringLiteral("text_alignment"),
                                 settings_ != nullptr ? settings_->textAlignment() : QStringLiteral("left"));
    return result == QStringLiteral("center") || result == QStringLiteral("right")
                || result == QStringLiteral("justify")
        ? result
        : QStringLiteral("left");
}

QString ReaderPreferencesController::imageFit() const
{
    const QString result = value(QStringLiteral("image_fit"),
                                 settings_ != nullptr ? settings_->imageFit() : QStringLiteral("page"));
    return result == QStringLiteral("width") || result == QStringLiteral("height")
                || result == QStringLiteral("actual")
        ? result
        : QStringLiteral("page");
}

QString ReaderPreferencesController::readingDirection() const
{
    return value(QStringLiteral("reading_direction"),
                 settings_ != nullptr ? settings_->readingDirection() : QStringLiteral("ltr"))
        == QStringLiteral("rtl")
        ? QStringLiteral("rtl")
        : QStringLiteral("ltr");
}

QString ReaderPreferencesController::customReaderBackground() const
{
    const QString result = value(QStringLiteral("custom_reader_background"),
                                 settings_ != nullptr ? settings_->customReaderBackground() : QString());
    return isHexColor(result) ? result : QString();
}

QString ReaderPreferencesController::customReaderText() const
{
    const QString result = value(QStringLiteral("custom_reader_text"),
                                 settings_ != nullptr ? settings_->customReaderText() : QString());
    return isHexColor(result) ? result : QString();
}

QString ReaderPreferencesController::customHighlightColor() const
{
    const QString result = value(QStringLiteral("custom_highlight_color"),
                                 settings_ != nullptr ? settings_->customHighlightColor() : QString());
    return isHexColor(result) ? result : QString();
}

QString ReaderPreferencesController::scope() const
{
    return scope_;
}

bool ReaderPreferencesController::contextAvailable() const
{
    return !format_.isEmpty() || !contentType_.isEmpty() || !documentId_.isEmpty();
}

void ReaderPreferencesController::refresh()
{
    emit preferencesChanged();
}

void ReaderPreferencesController::setContext(const QString &format,
                                             const QString &contentType,
                                             const QString &documentId)
{
    const QString normalizedFormat = format.trimmed().toLower();
    const QString normalizedContentType = contentType.trimmed().toLower();
    const QString normalizedDocumentId = documentId.trimmed().toLower();
    if (format_ == normalizedFormat && contentType_ == normalizedContentType
        && documentId_ == normalizedDocumentId) {
        return;
    }
    format_ = normalizedFormat;
    contentType_ = normalizedContentType;
    documentId_ = normalizedDocumentId;
    if (!contextAvailable() && scope_ != QStringLiteral("global")) {
        scope_ = QStringLiteral("global");
    }
    emit preferencesChanged();
}

void ReaderPreferencesController::setTheme(const QString &valueToStore)
{
    const QString normalized = valueToStore.trimmed().toLower();
    static const QStringList supported = {
        QStringLiteral("light"), QStringLiteral("dark"), QStringLiteral("sepia"),
        QStringLiteral("paper"), QStringLiteral("gray"), QStringLiteral("high-contrast"),
        QStringLiteral("low-light"),
    };
    if (!supported.contains(normalized)) {
        return;
    }
    setValue(QStringLiteral("theme"), normalized);
}

void ReaderPreferencesController::setFontSize(int valueToStore)
{
    setValue(QStringLiteral("font_size"), QString::number(std::clamp(valueToStore, 12, 32)));
}

void ReaderPreferencesController::setLineSpacing(int valueToStore)
{
    setValue(QStringLiteral("line_spacing"), QString::number(std::clamp(valueToStore, 100, 220)));
}

void ReaderPreferencesController::setContinuousReading(bool valueToStore)
{
    setValue(QStringLiteral("continuous_reading"), valueToStore ? QStringLiteral("1") : QStringLiteral("0"));
}

void ReaderPreferencesController::setHighContrast(bool valueToStore)
{
    setValue(QStringLiteral("high_contrast"), valueToStore ? QStringLiteral("1") : QStringLiteral("0"));
}

void ReaderPreferencesController::setFontFamily(const QString &value)
{
    const QString normalized = value.trimmed().left(64);
    if (!normalized.isEmpty()) {
        setValue(QStringLiteral("font_family"), normalized);
    }
}

void ReaderPreferencesController::setReaderWidth(int value)
{
    setValue(QStringLiteral("reader_width"), QString::number(std::clamp(value, 600, 1400)));
}

void ReaderPreferencesController::setReaderMargin(int value)
{
    setValue(QStringLiteral("reader_margin"), QString::number(std::clamp(value, 12, 80)));
}

void ReaderPreferencesController::setTextAlignment(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("left") || normalized == QStringLiteral("center")
        || normalized == QStringLiteral("right") || normalized == QStringLiteral("justify")) {
        setValue(QStringLiteral("text_alignment"), normalized);
    }
}

void ReaderPreferencesController::setImageFit(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("page") || normalized == QStringLiteral("width")
        || normalized == QStringLiteral("height") || normalized == QStringLiteral("actual")) {
        setValue(QStringLiteral("image_fit"), normalized);
    }
}

void ReaderPreferencesController::setReadingDirection(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("ltr") || normalized == QStringLiteral("rtl")) {
        setValue(QStringLiteral("reading_direction"), normalized);
    }
}

void ReaderPreferencesController::setCustomReaderBackground(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (isHexColor(normalized)) {
        setValue(QStringLiteral("custom_reader_background"), normalized);
    }
}

void ReaderPreferencesController::setCustomReaderText(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (isHexColor(normalized)) {
        setValue(QStringLiteral("custom_reader_text"), normalized);
    }
}

void ReaderPreferencesController::setCustomHighlightColor(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (isHexColor(normalized)) {
        setValue(QStringLiteral("custom_highlight_color"), normalized);
    }
}

void ReaderPreferencesController::setScope(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized != QStringLiteral("global") && normalized != QStringLiteral("format")
        && normalized != QStringLiteral("type") && normalized != QStringLiteral("document")) {
        return;
    }
    const QString nextScope = normalized == QStringLiteral("global") || contextAvailable()
        ? normalized
        : QStringLiteral("global");
    if (nextScope == scope_) {
        return;
    }
    scope_ = nextScope;
    emit preferencesChanged();
}

QString ReaderPreferencesController::value(const QString &key, const QString &fallback) const
{
    return settings_ != nullptr
        ? settings_->readerPreferenceValue(key, format_, contentType_, documentId_, fallback)
        : fallback;
}

void ReaderPreferencesController::setValue(const QString &key, const QString &valueToStore)
{
    if (settings_ != nullptr) {
        settings_->setReaderPreferenceValue(
            key, scope_, format_, contentType_, documentId_, valueToStore);
    }
    emit preferencesChanged();
}

} // namespace Lumina::Application

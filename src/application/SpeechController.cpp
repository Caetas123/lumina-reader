#include "application/SpeechController.h"

#include "infrastructure/database/Database.h"

#include <QSqlQuery>

#include <algorithm>

#ifdef LUMINA_HAS_QT_SPEECH
#include <QTextToSpeech>
#endif

namespace Lumina::Application {

SpeechController::SpeechController(QObject *parent)
    : QObject(parent)
{
#ifdef LUMINA_HAS_QT_SPEECH
    speech_ = std::make_unique<QTextToSpeech>(this);
    available_ = speech_ != nullptr;
    connect(speech_.get(), &QTextToSpeech::stateChanged, this, [this](QTextToSpeech::State state) {
        speaking_ = state == QTextToSpeech::Speaking || state == QTextToSpeech::Paused;
        paused_ = state == QTextToSpeech::Paused;
        if (state == QTextToSpeech::Error) {
            errorMessage_ = QStringLiteral("The system text-to-speech engine could not read this passage.");
        } else if (state == QTextToSpeech::Ready) {
            errorMessage_.clear();
        }
        emit stateChanged();
    });
#endif
}

SpeechController::~SpeechController() = default;

bool SpeechController::available() const
{
    return available_;
}

bool SpeechController::speaking() const
{
    return speaking_;
}

bool SpeechController::paused() const
{
    return paused_;
}

int SpeechController::rate() const
{
    return rate_;
}

int SpeechController::volume() const
{
    return volume_;
}

QString SpeechController::errorMessage() const
{
    return errorMessage_;
}

void SpeechController::setDatabase(Lumina::Infrastructure::Database *database)
{
    database_ = database;
    load();
}

void SpeechController::reload()
{
    load();
}

void SpeechController::setRate(int rate)
{
    const int normalized = std::clamp(rate, 50, 200);
    if (normalized == rate_) {
        return;
    }
    rate_ = normalized;
    persist(QStringLiteral("speech_rate"), QString::number(rate_));
#ifdef LUMINA_HAS_QT_SPEECH
    if (speech_ != nullptr) {
        speech_->setRate((static_cast<qreal>(rate_) - 100.0) / 100.0);
    }
#endif
    emit settingsChanged();
}

void SpeechController::setVolume(int volume)
{
    const int normalized = std::clamp(volume, 0, 100);
    if (normalized == volume_) {
        return;
    }
    volume_ = normalized;
    persist(QStringLiteral("speech_volume"), QString::number(volume_));
#ifdef LUMINA_HAS_QT_SPEECH
    if (speech_ != nullptr) {
        speech_->setVolume(static_cast<qreal>(volume_) / 100.0);
    }
#endif
    emit settingsChanged();
}

bool SpeechController::speak(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        errorMessage_ = QStringLiteral("Select or open a passage before starting read aloud.");
        emit stateChanged();
        return false;
    }
#ifdef LUMINA_HAS_QT_SPEECH
    if (speech_ != nullptr) {
        errorMessage_.clear();
        paused_ = false;
        speech_->say(trimmed);
        emit stateChanged();
        return true;
    }
#endif
    errorMessage_ = QStringLiteral("Read aloud is not available in this build.");
    emit stateChanged();
    return false;
}

void SpeechController::stop()
{
#ifdef LUMINA_HAS_QT_SPEECH
    if (speech_ != nullptr) {
        speech_->stop();
    }
#endif
    paused_ = false;
    speaking_ = false;
    emit stateChanged();
}

void SpeechController::pause()
{
#ifdef LUMINA_HAS_QT_SPEECH
    if (speech_ != nullptr) {
        speech_->pause();
    }
#endif
}

void SpeechController::resume()
{
#ifdef LUMINA_HAS_QT_SPEECH
    if (speech_ != nullptr) {
        speech_->resume();
    }
#endif
}

void SpeechController::load()
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
        const int value = query.value(1).toInt();
        if (key == QStringLiteral("speech_rate")) {
            rate_ = std::clamp(value, 50, 200);
        } else if (key == QStringLiteral("speech_volume")) {
            volume_ = std::clamp(value, 0, 100);
        }
    }
#ifdef LUMINA_HAS_QT_SPEECH
    if (speech_ != nullptr) {
        speech_->setRate((static_cast<qreal>(rate_) - 100.0) / 100.0);
        speech_->setVolume(static_cast<qreal>(volume_) / 100.0);
    }
#endif
    emit settingsChanged();
}

void SpeechController::persist(const QString &key, const QString &value)
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

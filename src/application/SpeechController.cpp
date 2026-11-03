#include "application/SpeechController.h"

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

void SpeechController::setRate(int rate)
{
    rate_ = std::clamp(rate, 50, 200);
#ifdef LUMINA_HAS_QT_SPEECH
    if (speech_ != nullptr) {
        speech_->setRate((static_cast<qreal>(rate_) - 100.0) / 100.0);
    }
#endif
    emit settingsChanged();
}

void SpeechController::setVolume(int volume)
{
    volume_ = std::clamp(volume, 0, 100);
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

} // namespace Lumina::Application

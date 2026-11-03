#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include <memory>

#ifdef LUMINA_HAS_QT_SPEECH
class QTextToSpeech;
#endif

namespace Lumina::Infrastructure {
class Database;
}

namespace Lumina::Application {

class SpeechController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool speaking READ speaking NOTIFY stateChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY stateChanged)
    Q_PROPERTY(int rate READ rate WRITE setRate NOTIFY settingsChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY settingsChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)

public:
    explicit SpeechController(QObject *parent = nullptr);
    ~SpeechController() override;

    [[nodiscard]] bool available() const;
    [[nodiscard]] bool speaking() const;
    [[nodiscard]] bool paused() const;
    [[nodiscard]] int rate() const;
    [[nodiscard]] int volume() const;
    [[nodiscard]] QString errorMessage() const;

    void setRate(int rate);
    void setVolume(int volume);
    void setDatabase(Lumina::Infrastructure::Database *database);

    Q_INVOKABLE bool speak(const QString &text);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();

signals:
    void stateChanged();
    void settingsChanged();

private:
    void load();
    void persist(const QString &key, const QString &value);

    bool available_ = false;
    bool speaking_ = false;
    bool paused_ = false;
    int rate_ = 100;
    int volume_ = 100;
    QString errorMessage_;
    Lumina::Infrastructure::Database *database_ = nullptr;

#ifdef LUMINA_HAS_QT_SPEECH
    std::unique_ptr<QTextToSpeech> speech_;
#endif
};

} // namespace Lumina::Application

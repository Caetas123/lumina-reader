#pragma once

#include "domain/ocr/OcrProvider.h"

#include <QProcess>
#include <QSize>
#include <QStringList>
#include <QTemporaryFile>

namespace Lumina::Infrastructure {

class TesseractOcrProvider final : public QObject, public Lumina::Domain::OcrProvider {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit TesseractOcrProvider(QObject *parent = nullptr);

    [[nodiscard]] QString name() const override;
    [[nodiscard]] bool isAvailable() const override;
    [[nodiscard]] QStringList availableLanguages() const;
    [[nodiscard]] bool busy() const;
    [[nodiscard]] static QStringList argumentsFor(const QString &imagePath,
                                                  const QString &language);
    [[nodiscard]] bool recognize(const QImage &image,
                                 const QString &language,
                                 Completion completion) override;
    void cancel() override;

signals:
    void busyChanged();

private slots:
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleError(QProcess::ProcessError error);

private:
    [[nodiscard]] QString executablePath() const;
    void finish(const Lumina::Domain::OcrProvider::Result &result, const QString &error);

    QProcess process_;
    QTemporaryFile imageFile_;
    Completion completion_;
    QSize imageSize_;
    bool busy_ = false;
};

} // namespace Lumina::Infrastructure

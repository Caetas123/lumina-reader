#pragma once

#include "domain/ocr/OcrProvider.h"

#include <QProcess>
#include <QTemporaryFile>

namespace Lumina::Infrastructure {

class TesseractOcrProvider final : public QObject, public Lumina::Domain::OcrProvider {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit TesseractOcrProvider(QObject *parent = nullptr);

    [[nodiscard]] QString name() const override;
    [[nodiscard]] bool isAvailable() const override;
    [[nodiscard]] bool busy() const;
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
    void finish(const QString &text, const QString &error);

    QProcess process_;
    QTemporaryFile imageFile_;
    Completion completion_;
    bool busy_ = false;
};

} // namespace Lumina::Infrastructure

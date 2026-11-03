#include "infrastructure/ocr/TesseractOcrProvider.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

namespace Lumina::Infrastructure {

TesseractOcrProvider::TesseractOcrProvider(QObject *parent)
    : QObject(parent)
{
    connect(&process_, &QProcess::finished, this, &TesseractOcrProvider::handleFinished);
    connect(&process_, &QProcess::errorOccurred, this, &TesseractOcrProvider::handleError);
}

QString TesseractOcrProvider::name() const
{
    return QStringLiteral("Tesseract OCR");
}

QString TesseractOcrProvider::executablePath() const
{
    const QString configuredPath = qEnvironmentVariable("LUMINA_TESSERACT_PATH").trimmed();
    if (!configuredPath.isEmpty() && QFileInfo::exists(configuredPath)) {
        return configuredPath;
    }
    return QStandardPaths::findExecutable(QStringLiteral("tesseract"));
}

bool TesseractOcrProvider::isAvailable() const
{
    return !executablePath().isEmpty();
}

bool TesseractOcrProvider::busy() const
{
    return busy_;
}

bool TesseractOcrProvider::recognize(const QImage &image,
                                    const QString &language,
                                    Completion completion)
{
    if (busy_ || image.isNull() || completion == nullptr) {
        return false;
    }

    const QString executable = executablePath();
    if (executable.isEmpty()) {
        completion({}, QStringLiteral(
            "Tesseract OCR is not installed. Install Tesseract and make it available on PATH."));
        return false;
    }

    const QString normalizedLanguage = language.trimmed().isEmpty()
        ? QStringLiteral("eng")
        : language.trimmed();
    if (normalizedLanguage.size() > 64
        || !normalizedLanguage.contains(QRegularExpression(QStringLiteral("^[A-Za-z0-9_+.-]+$")))) {
        completion({}, QStringLiteral("The OCR language code is invalid."));
        return false;
    }

    imageFile_.setAutoRemove(true);
    if (!imageFile_.open()) {
        completion({}, QStringLiteral("Lumina Reader could not prepare the image for OCR."));
        return false;
    }
    if (!image.save(&imageFile_, "PNG")) {
        imageFile_.close();
        completion({}, QStringLiteral("Lumina Reader could not prepare the image for OCR."));
        return false;
    }
    imageFile_.close();

    completion_ = std::move(completion);
    busy_ = true;
    emit busyChanged();

    process_.setProgram(executable);
    process_.setArguments({imageFile_.fileName(),
                           QStringLiteral("stdout"),
                           QStringLiteral("--language"),
                           normalizedLanguage,
                           QStringLiteral("--psm"),
                           QStringLiteral("3")});
    process_.start();
    return true;
}

void TesseractOcrProvider::cancel()
{
    if (!busy_) {
        return;
    }
    process_.kill();
    finish({}, QStringLiteral("OCR was cancelled."));
}

void TesseractOcrProvider::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!busy_) {
        return;
    }
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        const QString details = QString::fromLocal8Bit(process_.readAllStandardError()).trimmed();
        finish({}, details.isEmpty()
                       ? QStringLiteral("Tesseract could not recognize this page.")
                       : QStringLiteral("Tesseract could not recognize this page: %1").arg(details));
        return;
    }
    finish(QString::fromUtf8(process_.readAllStandardOutput()).trimmed(), {});
}

void TesseractOcrProvider::handleError(QProcess::ProcessError error)
{
    if (!busy_ || error == QProcess::UnknownError) {
        return;
    }
    finish({}, QStringLiteral("Lumina Reader could not start the OCR provider."));
}

void TesseractOcrProvider::finish(const QString &text, const QString &error)
{
    if (!busy_ && completion_ == nullptr) {
        return;
    }
    busy_ = false;
    emit busyChanged();
    imageFile_.close();
    const Completion completion = std::move(completion_);
    if (completion != nullptr) {
        completion(text, error);
    }
}

} // namespace Lumina::Infrastructure

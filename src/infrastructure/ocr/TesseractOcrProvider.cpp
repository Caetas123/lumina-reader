#include "infrastructure/ocr/TesseractOcrProvider.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>

#include <algorithm>

namespace {

Lumina::Domain::OcrProvider::Result parseTesseractTsv(const QByteArray &data, const QSize &imageSize)
{
    Lumina::Domain::OcrProvider::Result result;
    if (imageSize.isEmpty()) {
        return result;
    }

    QString previousLine;
    const QStringList lines = QString::fromUtf8(data).split(QChar('\n'), Qt::KeepEmptyParts);
    for (const QString &rawLine : lines) {
        const QStringList fields = rawLine.trimmed().split(QChar('\t'), Qt::KeepEmptyParts);
        if (fields.size() < 12 || fields.at(0) != QStringLiteral("5")) {
            continue;
        }
        const QString wordText = fields.at(11).trimmed();
        if (wordText.isEmpty()) {
            continue;
        }

        bool leftOk = false;
        bool topOk = false;
        bool widthOk = false;
        bool heightOk = false;
        const int left = fields.at(6).toInt(&leftOk);
        const int top = fields.at(7).toInt(&topOk);
        const int width = fields.at(8).toInt(&widthOk);
        const int height = fields.at(9).toInt(&heightOk);
        if (!leftOk || !topOk || !widthOk || !heightOk || width <= 0 || height <= 0) {
            continue;
        }

        const QString lineKey = fields.mid(1, 4).join(QChar(':'));
        if (!result.text.isEmpty()) {
            result.text += lineKey == previousLine ? QChar(' ') : QChar('\n');
        }
        const int start = result.text.size();
        result.text += wordText;
        result.words.append(Lumina::Domain::OcrProvider::Word{
            wordText,
            start,
            static_cast<int>(wordText.size()),
            std::clamp(static_cast<double>(left) / imageSize.width(), 0.0, 1.0),
            std::clamp(static_cast<double>(top) / imageSize.height(), 0.0, 1.0),
            std::clamp(static_cast<double>(width) / imageSize.width(), 0.0, 1.0),
            std::clamp(static_cast<double>(height) / imageSize.height(), 0.0, 1.0),
        });
        previousLine = lineKey;
    }
    return result;
}

} // namespace

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
    const QString bundledPath = QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("ocr/tesseract%1").arg(
#ifdef Q_OS_WIN
            QStringLiteral(".exe")
#else
            QString()
#endif
        ));
    if (QFileInfo::exists(bundledPath)) {
        return bundledPath;
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

QStringList TesseractOcrProvider::argumentsFor(const QString &imagePath,
                                               const QString &language)
{
    return {imagePath,
            QStringLiteral("stdout"),
            QStringLiteral("-l"),
            language,
            QStringLiteral("--psm"),
            QStringLiteral("3"),
            QStringLiteral("tsv")};
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
    process_.setArguments(argumentsFor(imageFile_.fileName(), normalizedLanguage));
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const QFileInfo executableInfo(executable);
    const QString bundledTessdata = executableInfo.dir().filePath(QStringLiteral("tessdata"));
    if (QFileInfo(bundledTessdata).isDir()) {
        environment.insert(QStringLiteral("TESSDATA_PREFIX"), bundledTessdata);
    }
    process_.setProcessEnvironment(environment);
    imageSize_ = image.size();
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
    finish(parseTesseractTsv(process_.readAllStandardOutput(), imageSize_), {});
}

void TesseractOcrProvider::handleError(QProcess::ProcessError error)
{
    if (!busy_ || error == QProcess::UnknownError) {
        return;
    }
    finish({}, QStringLiteral("Lumina Reader could not start the OCR provider."));
}

void TesseractOcrProvider::finish(const Lumina::Domain::OcrProvider::Result &result,
                                  const QString &error)
{
    if (!busy_ && completion_ == nullptr) {
        return;
    }
    busy_ = false;
    emit busyChanged();
    imageFile_.close();
    imageSize_ = {};
    const Completion completion = std::move(completion_);
    if (completion != nullptr) {
        completion(result, error);
    }
}

} // namespace Lumina::Infrastructure

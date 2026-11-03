#include "application/ReaderController.h"

#include "infrastructure/library/LibraryStore.h"

#include <QFile>
#include <QFileInfo>
#include <QUrl>

#include <algorithm>

namespace {

bool isTextFormat(const QString &format)
{
    return format == QStringLiteral("txt") || format == QStringLiteral("rtf")
        || format == QStringLiteral("fb2");
}

bool isImageFormat(const QString &format)
{
    return format == QStringLiteral("jpg") || format == QStringLiteral("jpeg")
        || format == QStringLiteral("png") || format == QStringLiteral("webp")
        || format == QStringLiteral("avif") || format == QStringLiteral("gif");
}

} // namespace

namespace Lumina::Application {

ReaderController::ReaderController(Lumina::Infrastructure::LibraryStore *library,
                                   QObject *parent)
    : QObject(parent)
    , library_(library)
{
}

bool ReaderController::isOpen() const
{
    return open_;
}

QString ReaderController::documentId() const
{
    return documentId_;
}

QString ReaderController::title() const
{
    return title_;
}

QString ReaderController::filePath() const
{
    return filePath_;
}

QString ReaderController::format() const
{
    return format_;
}

QString ReaderController::contentType() const
{
    return contentType_;
}

QString ReaderController::textContent() const
{
    return textContent_;
}

QString ReaderController::imageSource() const
{
    return imageSource_;
}

bool ReaderController::isImageDocument() const
{
    return isImageFormat(format_);
}

bool ReaderController::isTextDocument() const
{
    return isTextFormat(format_);
}

QString ReaderController::engineName() const
{
    return engineName_;
}

QString ReaderController::errorMessage() const
{
    return errorMessage_;
}

int ReaderController::pageCount() const
{
    return pageCount_;
}

int ReaderController::currentPage() const
{
    return currentPage_;
}

void ReaderController::clearState()
{
    open_ = false;
    documentId_.clear();
    title_.clear();
    filePath_.clear();
    format_.clear();
    contentType_.clear();
    textContent_.clear();
    imageSource_.clear();
    engineName_.clear();
    errorMessage_.clear();
    pageCount_ = 0;
    currentPage_ = 0;
}

void ReaderController::fail(const QString &message)
{
    clearState();
    errorMessage_ = message;
    emit stateChanged();
}

QString ReaderController::loadText(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

bool ReaderController::openDocument(const QString &documentId)
{
    if (library_ == nullptr) {
        fail(QStringLiteral("The library is not available."));
        return false;
    }

    const QVariantMap record = library_->documentDetails(documentId);
    if (record.isEmpty()) {
        fail(QStringLiteral("The selected document could not be opened."));
        return false;
    }

    const QString path = record.value(QStringLiteral("filePath")).toString();
    const QString format = record.value(QStringLiteral("format")).toString();
    if (!QFileInfo::exists(path)) {
        fail(QStringLiteral("The original file is no longer available at its saved location."));
        return false;
    }

    clearState();
    documentId_ = documentId;
    title_ = record.value(QStringLiteral("title")).toString();
    filePath_ = path;
    format_ = format;
    contentType_ = record.value(QStringLiteral("contentType")).toString();
    currentPage_ = std::max(record.value(QStringLiteral("currentPage")).toInt(), 0);

    if (isImageFormat(format)) {
        imageSource_ = QUrl::fromLocalFile(path).toString();
        pageCount_ = 1;
        currentPage_ = 0;
        engineName_ = QStringLiteral("Image engine");
    } else if (isTextFormat(format)) {
        textContent_ = loadText(path);
        if (textContent_.isEmpty() && QFileInfo(path).size() > 0) {
            fail(QStringLiteral("The text document could not be decoded."));
            return false;
        }
        constexpr int charactersPerPage = 5000;
        const int characterCount = static_cast<int>(textContent_.size());
        pageCount_ = std::max(1, (characterCount + charactersPerPage - 1) / charactersPerPage);
        currentPage_ = std::clamp(currentPage_, 0, pageCount_ - 1);
        engineName_ = QStringLiteral("Text engine");
    } else {
        fail(QStringLiteral(
            "This format is registered in the library, but its reading engine is not available in this build yet."));
        return false;
    }

    open_ = true;
    errorMessage_.clear();
    persistProgress();
    emit stateChanged();
    return true;
}

void ReaderController::closeDocument()
{
    if (!open_) {
        return;
    }
    persistProgress();
    clearState();
    emit stateChanged();
}

bool ReaderController::nextPage()
{
    return goToPage(currentPage_ + 1);
}

bool ReaderController::previousPage()
{
    return goToPage(currentPage_ - 1);
}

bool ReaderController::goToPage(int page)
{
    if (!open_ || page < 0 || page >= pageCount_) {
        return false;
    }
    if (page == currentPage_) {
        return true;
    }
    currentPage_ = page;
    persistProgress();
    emit stateChanged();
    return true;
}

bool ReaderController::updateProgress(double progress)
{
    if (!open_) {
        return false;
    }
    progress = std::clamp(progress, 0.0, 1.0);
    currentPage_ = std::clamp(static_cast<int>(progress * pageCount_), 0, std::max(pageCount_ - 1, 0));
    const bool success = library_ != nullptr
        && library_->updateProgress(documentId_, progress, currentPage_);
    if (success) {
        emit stateChanged();
    }
    return success;
}

void ReaderController::persistProgress()
{
    if (library_ == nullptr || !open_ || pageCount_ <= 0) {
        return;
    }
    const double progress = pageCount_ <= 1
        ? 1.0
        : static_cast<double>(currentPage_) / static_cast<double>(pageCount_ - 1);
    library_->updateProgress(documentId_, progress, currentPage_);
}

} // namespace Lumina::Application

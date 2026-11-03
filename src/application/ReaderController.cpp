#include "application/ReaderController.h"

#include "domain/reader/DocumentEngine.h"
#include "infrastructure/library/LibraryStore.h"
#include "infrastructure/ocr/TesseractOcrProvider.h"
#include "infrastructure/reader/DocumentEngineFactory.h"

#include <QBuffer>
#include <QFileInfo>
#include <QImage>
#include <QStringList>
#include <QTransform>

#include <algorithm>

namespace {

QString imageDataUrl(const QImage &image)
{
    QByteArray encoded;
    QBuffer buffer(&encoded);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) {
        return {};
    }
    return QStringLiteral("data:image/png;base64,") + QString::fromLatin1(encoded.toBase64());
}

} // namespace

namespace Lumina::Application {

ReaderController::ReaderController(Lumina::Infrastructure::LibraryStore *library,
                                   QObject *parent)
    : QObject(parent)
    , library_(library)
    , ocrProvider_(std::make_unique<Lumina::Infrastructure::TesseractOcrProvider>())
{
    // The provider is owned directly by the reader controller; its state is
    // reported by the reader after each OCR request.
    ocrProvider_->setObjectName(QStringLiteral("Lumina Tesseract OCR"));
}

ReaderController::~ReaderController()
{
    if (open_) {
        persistProgress();
    }
    if (ocrProvider_ != nullptr && ocrProvider_->busy()) {
        ocrProvider_->cancel();
    }
    ocrProvider_.reset();
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

QString ReaderController::continuousTextContent() const
{
    return continuousTextContent_;
}

QString ReaderController::imageSource() const
{
    return imageSource_;
}

bool ReaderController::isImageDocument() const
{
    return documentEngine_ != nullptr
        && documentEngine_->pageKind(currentPage_) == Lumina::Domain::Reader::PageKind::Image;
}

bool ReaderController::isTextDocument() const
{
    return showOcrText_
        || (documentEngine_ != nullptr
            && documentEngine_->pageKind(currentPage_) == Lumina::Domain::Reader::PageKind::Text);
}

bool ReaderController::pageTextAvailable() const
{
    return !textContent_.trimmed().isEmpty();
}

QString ReaderController::engineName() const
{
    return engineName_;
}

QString ReaderController::errorMessage() const
{
    return errorMessage_;
}

bool ReaderController::ocrAvailable() const
{
    return ocrProvider_ != nullptr && ocrProvider_->isAvailable();
}

bool ReaderController::ocrRunning() const
{
    return ocrProvider_ != nullptr && ocrProvider_->busy();
}

QString ReaderController::ocrStatus() const
{
    return ocrStatus_;
}

bool ReaderController::showOcrText() const
{
    return showOcrText_;
}

void ReaderController::setShowOcrText(bool show)
{
    if (showOcrText_ == show) {
        return;
    }
    showOcrText_ = show && !textContent_.isEmpty();
    emit stateChanged();
}

int ReaderController::pageCount() const
{
    return pageCount_;
}

int ReaderController::currentPage() const
{
    return currentPage_;
}

int ReaderController::pageRotation() const
{
    return pageRotation_;
}

int ReaderController::thumbnailRevision() const
{
    return thumbnailRevision_;
}

QString ReaderController::thumbnailSource(int page) const
{
    if (!open_ || documentEngine_ == nullptr || page < 0 || page >= pageCount_
        || documentEngine_->pageKind(page) != Lumina::Domain::Reader::PageKind::Image) {
        return {};
    }
    if (thumbnailCache_.contains(page)) {
        return thumbnailCache_.value(page);
    }

    Lumina::Domain::Error error;
    QImage image = documentEngine_->renderPage(page, QSize(180, 240), &error);
    if (image.isNull()) {
        return {};
    }
    if (pageRotation_ != 0) {
        image = image.transformed(QTransform().rotate(pageRotation_), Qt::SmoothTransformation);
    }
    const QString source = imageDataUrl(image);
    if (!source.isEmpty()) {
        thumbnailCache_.insert(page, source);
    }
    return source;
}

QString ReaderController::searchQuery() const
{
    return searchQuery_;
}

int ReaderController::searchMatches() const
{
    return searchStarts_.size();
}

int ReaderController::searchCurrent() const
{
    return searchCurrent_;
}

int ReaderController::searchStart() const
{
    return searchStart_;
}

int ReaderController::searchLength() const
{
    return searchLength_;
}

void ReaderController::clearState()
{
    ocrDocumentId_.clear();
    ocrLanguage_.clear();
    ocrPage_ = -1;
    if (ocrProvider_ != nullptr && ocrProvider_->busy()) {
        ocrProvider_->cancel();
    }
    if (documentEngine_ != nullptr) {
        documentEngine_->close();
        documentEngine_.reset();
    }
    open_ = false;
    documentId_.clear();
    title_.clear();
    filePath_.clear();
    format_.clear();
    contentType_.clear();
    textContent_.clear();
    continuousTextContent_.clear();
    imageSource_.clear();
    engineName_.clear();
    errorMessage_.clear();
    ocrStatus_.clear();
    showOcrText_ = false;
    pageCount_ = 0;
    currentPage_ = 0;
    pageRotation_ = 0;
    searchQuery_.clear();
    searchStarts_.clear();
    searchCurrent_ = -1;
    searchStart_ = -1;
    searchLength_ = 0;
    thumbnailCache_.clear();
    ++thumbnailRevision_;
    readingTimer_.invalidate();
}

void ReaderController::fail(const QString &message)
{
    clearState();
    errorMessage_ = message;
    emit stateChanged();
}

bool ReaderController::refreshPage()
{
    if (documentEngine_ == nullptr || currentPage_ < 0 || currentPage_ >= pageCount_) {
        return false;
    }

    Lumina::Domain::Error error;
    if (documentEngine_->pageKind(currentPage_)
        == Lumina::Domain::Reader::PageKind::Image) {
        QImage image = documentEngine_->renderPage(currentPage_, QSize(2200, 3100), &error);
        if (image.isNull()) {
            return false;
        }
        if (pageRotation_ != 0) {
            image = image.transformed(QTransform().rotate(pageRotation_), Qt::SmoothTransformation);
        }
        imageSource_ = imageDataUrl(image);
        if (imageSource_.isEmpty()) {
            return false;
        }
        textContent_ = documentEngine_->textPage(currentPage_, &error);
    } else {
        textContent_ = documentEngine_->textPage(currentPage_, &error);
        if (error.hasError()) {
            return false;
        }
        imageSource_.clear();
    }
    rebuildSearchMatches();
    return true;
}

void ReaderController::rebuildContinuousText()
{
    continuousTextContent_.clear();
    if (documentEngine_ == nullptr || pageCount_ <= 0) {
        return;
    }

    QStringList pages;
    for (int page = 0; page < pageCount_; ++page) {
        Lumina::Domain::Error error;
        const QString pageText = documentEngine_->textPage(page, &error);
        if (error.hasError()) {
            if (documentEngine_->pageKind(page) == Lumina::Domain::Reader::PageKind::Text) {
                continuousTextContent_.clear();
                return;
            }
            continue;
        }
        if (!pageText.isEmpty()) {
            pages.append(pageText);
        }
    }
    continuousTextContent_ = pages.join(QStringLiteral("\n\n"));
}

void ReaderController::applyCachedOcr()
{
    const bool imagePage = isImageDocument();
    if (!imagePage || library_ == nullptr) {
        showOcrText_ = false;
        return;
    }

    const QString cachedText = library_->ocrTextForPage(documentId_, currentPage_);
    if (cachedText.isEmpty()) {
        showOcrText_ = false;
        ocrStatus_.clear();
        return;
    }
    textContent_ = cachedText;
    showOcrText_ = true;
    ocrStatus_ = QStringLiteral("Cached OCR text is available for this page.");
    rebuildSearchMatches();
}

QImage ReaderController::currentPageImage() const
{
    if (documentEngine_ != nullptr && isImageDocument()) {
        QImage image = documentEngine_->renderPage(currentPage_, QSize(2200, 3100));
        if (!image.isNull() && pageRotation_ != 0) {
            image = image.transformed(QTransform().rotate(pageRotation_), Qt::SmoothTransformation);
        }
        return image;
    }
    return {};
}

void ReaderController::rebuildSearchMatches()
{
    searchStarts_.clear();
    searchCurrent_ = -1;
    searchStart_ = -1;
    searchLength_ = searchQuery_.size();
    if (searchQuery_.isEmpty() || textContent_.isEmpty()) {
        return;
    }

    qsizetype offset = 0;
    while (offset < textContent_.size()) {
        const qsizetype match = textContent_.indexOf(searchQuery_, offset, Qt::CaseInsensitive);
        if (match < 0) {
            break;
        }
        searchStarts_.append(static_cast<int>(match));
        offset = match + 1;
    }
}

bool ReaderController::selectSearchMatch(int index)
{
    if (searchStarts_.isEmpty()) {
        searchCurrent_ = -1;
        searchStart_ = -1;
        searchLength_ = searchQuery_.size();
        return false;
    }
    const int normalized = (index % searchStarts_.size() + searchStarts_.size())
        % searchStarts_.size();
    searchCurrent_ = normalized;
    searchStart_ = searchStarts_.at(normalized);
    searchLength_ = searchQuery_.size();
    return true;
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
    pageRotation_ = record.value(QStringLiteral("pageRotation")).toInt();
    if (pageRotation_ != 0 && pageRotation_ != 90 && pageRotation_ != 180 && pageRotation_ != 270) {
        pageRotation_ = 0;
    }

    documentEngine_ = Lumina::Infrastructure::createDocumentEngine(format);
    if (documentEngine_ == nullptr) {
        fail(QStringLiteral(
            "This format is registered in the library, but its reading engine is not available in this build yet."));
        return false;
    }

    Lumina::Domain::Error engineError;
    if (!documentEngine_->open(path, &engineError) || documentEngine_->pageCount() <= 0) {
        fail(engineError.hasError()
                 ? engineError.userMessage()
                 : QStringLiteral("The document could not be opened. It may be damaged or unsupported."));
        return false;
    }
    pageCount_ = documentEngine_->pageCount();
    currentPage_ = std::clamp(currentPage_, 0, pageCount_ - 1);
    engineName_ = documentEngine_->name();
    rebuildContinuousText();
    if (!refreshPage()) {
        fail(QStringLiteral("The first page of this document could not be read."));
        return false;
    }

    applyCachedOcr();

    open_ = true;
    readingTimer_.start();
    errorMessage_.clear();
    library_->setPageCount(documentId_, pageCount_);
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

QString ReaderController::pageName(int page) const
{
    if (!open_ || documentEngine_ == nullptr || page < 0 || page >= pageCount_) {
        return {};
    }
    return documentEngine_->pageName(page);
}

bool ReaderController::rotatePage(int degrees)
{
    if (!open_ || !isImageDocument() || library_ == nullptr) {
        return false;
    }
    int normalizedDegrees = degrees % 360;
    if (normalizedDegrees < 0) {
        normalizedDegrees += 360;
    }
    if (normalizedDegrees != 90 && normalizedDegrees != 180 && normalizedDegrees != 270) {
        return false;
    }
    const int nextRotation = (pageRotation_ + normalizedDegrees) % 360;
    if (!library_->setPageRotation(documentId_, nextRotation)) {
        return false;
    }
    pageRotation_ = nextRotation;
    thumbnailCache_.clear();
    ++thumbnailRevision_;
    if (!refreshPage()) {
        return false;
    }
    applyCachedOcr();
    emit stateChanged();
    return true;
}

bool ReaderController::goToPage(int page)
{
    if (!open_ || page < 0 || page >= pageCount_) {
        return false;
    }
    if (page == currentPage_) {
        return true;
    }
    const int previousPage = currentPage_;
    currentPage_ = page;
    if (!refreshPage()) {
        currentPage_ = previousPage;
        (void)refreshPage();
        return false;
    }
    applyCachedOcr();
    persistProgress();
    emit stateChanged();
    return true;
}

bool ReaderController::startOcr(const QString &language)
{
    if (!open_ || ocrProvider_ == nullptr) {
        return false;
    }
    if (ocrProvider_->busy()) {
        return false;
    }

    const QImage image = currentPageImage();
    if (image.isNull()) {
        ocrStatus_ = QStringLiteral("OCR is only available for image-based pages.");
        emit stateChanged();
        return false;
    }
    if (!ocrProvider_->isAvailable()) {
        ocrStatus_ = QStringLiteral(
            "Tesseract OCR is not installed. Install it and make it available on PATH.");
        emit stateChanged();
        return false;
    }

    ocrDocumentId_ = documentId_;
    ocrLanguage_ = language.trimmed().isEmpty() ? QStringLiteral("eng") : language.trimmed();
    ocrPage_ = currentPage_;
    ocrStatus_ = QStringLiteral("Recognizing text in this page…");
    const bool started = ocrProvider_->recognize(
        image,
        language,
        [this](const QString &text, const QString &error) {
            const QString targetDocumentId = ocrDocumentId_;
            const QString targetLanguage = ocrLanguage_;
            const int targetPage = ocrPage_;
            ocrDocumentId_.clear();
            ocrLanguage_.clear();
            ocrPage_ = -1;
            if (targetDocumentId.isEmpty() || library_ == nullptr) {
                return;
            }
            if (!error.isEmpty()) {
                ocrStatus_ = error;
                emit stateChanged();
                return;
            }
            if (!library_->saveOcrPage(targetDocumentId,
                                       targetPage,
                                       targetLanguage,
                                       text,
                                       QStringLiteral("completed"))) {
                ocrStatus_ = QStringLiteral("The recognized text could not be saved.");
                emit stateChanged();
                return;
            }
            if (targetDocumentId == documentId_ && targetPage == currentPage_) {
                textContent_ = text;
                showOcrText_ = !text.isEmpty();
                rebuildSearchMatches();
            }
            ocrStatus_ = text.isEmpty()
                ? QStringLiteral("No text was recognized on this page.")
                : QStringLiteral("Text recognized and saved to the Knowledge center.");
            emit stateChanged();
        });
    if (!started && ocrDocumentId_ == documentId_) {
        ocrDocumentId_.clear();
        ocrPage_ = -1;
        if (ocrStatus_.isEmpty()) {
            ocrStatus_ = QStringLiteral("OCR could not be started.");
        }
        emit stateChanged();
    }
    if (started) {
        emit stateChanged();
    }
    return started;
}

void ReaderController::cancelOcr()
{
    if (ocrProvider_ == nullptr || !ocrProvider_->busy()) {
        return;
    }
    ocrDocumentId_.clear();
    ocrLanguage_.clear();
    ocrPage_ = -1;
    ocrProvider_->cancel();
    ocrStatus_ = QStringLiteral("OCR was cancelled.");
    emit stateChanged();
}

bool ReaderController::updateProgress(double progress)
{
    if (!open_) {
        return false;
    }
    progress = std::clamp(progress, 0.0, 1.0);
    currentPage_ = std::clamp(static_cast<int>(progress * std::max(pageCount_ - 1, 0)),
                              0,
                              std::max(pageCount_ - 1, 0));
    if (!refreshPage()) {
        return false;
    }
    applyCachedOcr();
    const bool success = library_ != nullptr
        && library_->updateProgress(documentId_, progress, currentPage_);
    if (success) {
        emit stateChanged();
    }
    return success;
}

bool ReaderController::findInDocument(const QString &query)
{
    const QString normalized = query.trimmed();
    if (normalized == searchQuery_) {
        return !searchStarts_.isEmpty();
    }
    searchQuery_ = normalized;
    rebuildSearchMatches();
    const bool found = selectSearchMatch(0);
    emit stateChanged();
    return found;
}

bool ReaderController::nextSearchMatch()
{
    if (searchStarts_.isEmpty()) {
        return false;
    }
    const bool found = selectSearchMatch(searchCurrent_ + 1);
    emit stateChanged();
    return found;
}

bool ReaderController::previousSearchMatch()
{
    if (searchStarts_.isEmpty()) {
        return false;
    }
    const bool found = selectSearchMatch(searchCurrent_ - 1);
    emit stateChanged();
    return found;
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
    persistReadingTime();
}

void ReaderController::persistReadingTime()
{
    if (library_ == nullptr || !open_ || !readingTimer_.isValid()) {
        return;
    }
    const qint64 seconds = readingTimer_.elapsed() / 1000;
    if (seconds <= 0) {
        return;
    }
    if (library_->addReadingTime(documentId_, seconds)) {
        readingTimer_.restart();
    }
}

} // namespace Lumina::Application

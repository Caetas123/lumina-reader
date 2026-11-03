#include "application/ReaderController.h"

#include "infrastructure/archive/ArchiveReader.h"
#include "infrastructure/library/LibraryStore.h"
#include "infrastructure/text/TextDecoder.h"

#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QImage>
#include <QUrl>

#ifdef LUMINA_HAS_QT_PDF
#include <QPdfDocument>
#endif

#include <algorithm>

namespace {

constexpr int kTextCharactersPerPage = 5000;

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

bool isArchiveFormat(const QString &format)
{
    return format == QStringLiteral("cbz") || format == QStringLiteral("zip")
        || format == QStringLiteral("epub");
}

QString imageMimeType(const QString &path)
{
    const QString suffix = path.section('.', -1).toLower();
    if (suffix == QStringLiteral("jpg") || suffix == QStringLiteral("jpeg")) {
        return QStringLiteral("image/jpeg");
    }
    if (suffix == QStringLiteral("png")) {
        return QStringLiteral("image/png");
    }
    if (suffix == QStringLiteral("webp")) {
        return QStringLiteral("image/webp");
    }
    if (suffix == QStringLiteral("gif")) {
        return QStringLiteral("image/gif");
    }
    if (suffix == QStringLiteral("avif")) {
        return QStringLiteral("image/avif");
    }
    return QStringLiteral("application/octet-stream");
}

QString imageDataUrl(const QString &path, const QByteArray &data)
{
    return imageMimeType(path) + QStringLiteral(";base64,")
        + QString::fromLatin1(data.toBase64());
}

} // namespace

namespace Lumina::Application {

ReaderController::ReaderController(Lumina::Infrastructure::LibraryStore *library,
                                   QObject *parent)
    : QObject(parent)
    , library_(library)
{
}

ReaderController::~ReaderController() = default;

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
    return isImageFormat(format_) || format_ == QStringLiteral("pdf")
        || (archiveReader_ != nullptr && archiveReader_->pageIsImage(currentPage_));
}

bool ReaderController::isTextDocument() const
{
    return isTextFormat(format_)
        || (archiveReader_ != nullptr && !archiveReader_->pageIsImage(currentPage_));
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
    archiveReader_.reset();
#ifdef LUMINA_HAS_QT_PDF
    pdfDocument_.reset();
#endif
    open_ = false;
    documentId_.clear();
    title_.clear();
    filePath_.clear();
    format_.clear();
    contentType_.clear();
    fullTextContent_.clear();
    textContent_.clear();
    imageSource_.clear();
    engineName_.clear();
    errorMessage_.clear();
    pageCount_ = 0;
    currentPage_ = 0;
    searchQuery_.clear();
    searchStarts_.clear();
    searchCurrent_ = -1;
    searchStart_ = -1;
    searchLength_ = 0;
}

void ReaderController::fail(const QString &message)
{
    clearState();
    errorMessage_ = message;
    emit stateChanged();
}

QString ReaderController::loadText(const QString &path) const
{
    return Lumina::Infrastructure::TextDecoder::readFile(path, format_);
}

void ReaderController::refreshTextPage()
{
    if (!isTextFormat(format_)) {
        return;
    }
    const qsizetype start = static_cast<qsizetype>(currentPage_) * kTextCharactersPerPage;
    textContent_ = fullTextContent_.mid(start, kTextCharactersPerPage);
    rebuildSearchMatches();
}

bool ReaderController::refreshArchivePage()
{
    if (archiveReader_ == nullptr || currentPage_ < 0 || currentPage_ >= pageCount_) {
        return false;
    }

    if (archiveReader_->pageIsImage(currentPage_)) {
        const QByteArray data = archiveReader_->pageData(currentPage_);
        if (data.isEmpty()) {
            return false;
        }
        imageSource_ = imageDataUrl(archiveReader_->pageName(currentPage_), data);
        textContent_.clear();
    } else {
        textContent_ = archiveReader_->pageText(currentPage_);
        if (textContent_.isEmpty() && !archiveReader_->pageData(currentPage_).isEmpty()) {
            return false;
        }
        imageSource_.clear();
    }
    rebuildSearchMatches();
    return true;
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

bool ReaderController::renderPdfPage()
{
#ifdef LUMINA_HAS_QT_PDF
    if (pdfDocument_ == nullptr || currentPage_ < 0 || currentPage_ >= pageCount_) {
        return false;
    }
    const QImage rendered = pdfDocument_->render(currentPage_, QSize(1500, 2100));
    if (rendered.isNull()) {
        return false;
    }
    QByteArray encoded;
    QBuffer buffer(&encoded);
    if (!buffer.open(QIODevice::WriteOnly) || !rendered.save(&buffer, "PNG")) {
        return false;
    }
    imageSource_ = QStringLiteral("data:image/png;base64,")
        + QString::fromLatin1(encoded.toBase64());
    const auto selection = pdfDocument_->getAllText(currentPage_);
    textContent_ = selection.isValid() ? selection.text() : QString();
    rebuildSearchMatches();
    return true;
#else
    return false;
#endif
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
    } else if (format == QStringLiteral("pdf")) {
#ifdef LUMINA_HAS_QT_PDF
        pdfDocument_ = std::make_unique<QPdfDocument>();
        const QPdfDocument::Error error = pdfDocument_->load(path);
        if (error != QPdfDocument::Error::None || pdfDocument_->pageCount() <= 0) {
            fail(QStringLiteral("The PDF could not be opened. It may be damaged, encrypted, or unsupported."));
            return false;
        }
        pageCount_ = pdfDocument_->pageCount();
        currentPage_ = std::clamp(currentPage_, 0, pageCount_ - 1);
        if (!renderPdfPage()) {
            fail(QStringLiteral("The first PDF page could not be rendered."));
            return false;
        }
        engineName_ = QStringLiteral("Qt PDF engine");
#else
        fail(QStringLiteral("PDF support is not included in this build."));
        return false;
#endif
    } else if (isTextFormat(format)) {
        fullTextContent_ = loadText(path);
        if (fullTextContent_.isEmpty() && QFileInfo(path).size() > 0) {
            fail(QStringLiteral("The text document could not be decoded."));
            return false;
        }
        const int characterCount = static_cast<int>(fullTextContent_.size());
        pageCount_ = std::max(1, (characterCount + kTextCharactersPerPage - 1)
                                      / kTextCharactersPerPage);
        currentPage_ = std::clamp(currentPage_, 0, pageCount_ - 1);
        refreshTextPage();
        engineName_ = QStringLiteral("Text engine");
    } else if (isArchiveFormat(format)) {
        archiveReader_ = std::make_unique<Lumina::Infrastructure::ArchiveReader>();
        if (!archiveReader_->open(path, format)) {
            fail(QStringLiteral("This archive could not be opened. It may be damaged or unsupported."));
            return false;
        }
        pageCount_ = archiveReader_->pageCount();
        currentPage_ = std::clamp(currentPage_, 0, pageCount_ - 1);
        if (!refreshArchivePage()) {
            fail(QStringLiteral("The first archive page could not be read."));
            return false;
        }
        engineName_ = QStringLiteral("Archive engine");
    } else {
        fail(QStringLiteral(
            "This format is registered in the library, but its reading engine is not available in this build yet."));
        return false;
    }

    open_ = true;
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
    if (isTextFormat(format_)) {
        refreshTextPage();
    }
    if (archiveReader_ != nullptr && !refreshArchivePage()) {
        currentPage_ = previousPage;
        refreshArchivePage();
        return false;
    }
#ifdef LUMINA_HAS_QT_PDF
    if (format_ == QStringLiteral("pdf") && !renderPdfPage()) {
        currentPage_ = previousPage;
        return false;
    }
#endif
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
    currentPage_ = std::clamp(static_cast<int>(progress * std::max(pageCount_ - 1, 0)),
                              0,
                              std::max(pageCount_ - 1, 0));
    refreshTextPage();
    if (archiveReader_ != nullptr && !refreshArchivePage()) {
        return false;
    }
#ifdef LUMINA_HAS_QT_PDF
    if (format_ == QStringLiteral("pdf") && !renderPdfPage()) {
        return false;
    }
#endif
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
}

} // namespace Lumina::Application

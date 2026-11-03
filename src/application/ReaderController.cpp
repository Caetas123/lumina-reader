#include "application/ReaderController.h"

#include "domain/ocr/OcrProvider.h"
#include "domain/reader/DocumentEngine.h"
#include "infrastructure/library/LibraryStore.h"
#include "infrastructure/ocr/TesseractOcrProvider.h"
#include "infrastructure/reader/DocumentEngineFactory.h"

#include <QBuffer>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QPageSize>
#include <QPen>
#include <QPagedPaintDevice>
#include <QPdfWriter>
#include <QPrintDialog>
#include <QPrinter>
#include <QSaveFile>
#include <QStringList>
#include <QTextDocument>
#include <QTimer>
#include <QTransform>

#include <algorithm>
#include <utility>

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

bool sameFilePath(const QString &first, const QString &second)
{
    const QString firstPath = QDir::cleanPath(QFileInfo(first).absoluteFilePath());
    const QString secondPath = QDir::cleanPath(QFileInfo(second).absoluteFilePath());
#ifdef Q_OS_WIN
    constexpr Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseInsensitive;
#else
    constexpr Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseSensitive;
#endif
    return firstPath.compare(secondPath, pathCaseSensitivity) == 0;
}

QVariantList ocrWordsAsVariants(const Lumina::Domain::OcrProvider::Result &result)
{
    QVariantList words;
    for (const Lumina::Domain::OcrProvider::Word &word : result.words) {
        words.append(QVariantMap{
            {QStringLiteral("text"), word.text},
            {QStringLiteral("start"), word.start},
            {QStringLiteral("length"), word.length},
            {QStringLiteral("x"), word.x},
            {QStringLiteral("y"), word.y},
            {QStringLiteral("width"), word.width},
            {QStringLiteral("height"), word.height},
        });
    }
    return words;
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
    return ocrDocumentRunning_ || (ocrProvider_ != nullptr && ocrProvider_->busy());
}

bool ReaderController::ocrDocumentRunning() const
{
    return ocrDocumentRunning_;
}

double ReaderController::ocrProgress() const
{
    if (ocrBatchPages_.isEmpty()) {
        return 0.0;
    }
    return std::clamp(static_cast<double>(ocrBatchIndex_)
                          / static_cast<double>(ocrBatchPages_.size()),
                      0.0,
                      1.0);
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
    ocrDocumentRunning_ = false;
    ocrBatchDocumentId_.clear();
    ocrBatchLanguage_.clear();
    ocrBatchPages_.clear();
    ocrBatchIndex_ = 0;
    ocrBatchSucceeded_ = 0;
    ocrBatchFailed_ = 0;
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
    pageImageCache_.clear();
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

QImage ReaderController::pageImage(int page) const
{
    if (documentEngine_ != nullptr && page >= 0 && page < pageCount_
        && documentEngine_->pageKind(page) == Lumina::Domain::Reader::PageKind::Image) {
        QImage image = documentEngine_->renderPage(page, QSize(2200, 3100));
        if (!image.isNull() && pageRotation_ != 0) {
            image = image.transformed(QTransform().rotate(pageRotation_), Qt::SmoothTransformation);
        }
        return image;
    }
    return {};
}

QImage ReaderController::currentPageImage() const
{
    return pageImage(currentPage_);
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

    // Page navigation rebuilds the matches while keeping the query visible in
    // the toolbar. Select the first result immediately so the counter and the
    // highlight never remain at an invalid 0/N state after returning to a
    // matching page.
    if (!searchStarts_.isEmpty()) {
        const bool selected = selectSearchMatch(0);
        Q_UNUSED(selected)
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

QString ReaderController::pageImageSource(int page) const
{
    if (!open_ || documentEngine_ == nullptr || page < 0 || page >= pageCount_
        || documentEngine_->pageKind(page) != Lumina::Domain::Reader::PageKind::Image) {
        return {};
    }
    if (page == currentPage_) {
        return imageSource_;
    }
    if (pageImageCache_.contains(page)) {
        return pageImageCache_.value(page);
    }

    const QString source = imageDataUrl(pageImage(page));
    if (!source.isEmpty()) {
        pageImageCache_.insert(page, source);
    }
    return source;
}

bool ReaderController::exportText(const QString &path)
{
    if (!open_) {
        return false;
    }
    const QString normalizedPath = path.trimmed();
    const QFileInfo outputInfo(normalizedPath);
    if (normalizedPath.isEmpty() || !outputInfo.absoluteDir().exists()) {
        errorMessage_ = QStringLiteral("Choose a valid destination folder for the text export.");
        emit stateChanged();
        return false;
    }

    if (sameFilePath(normalizedPath, filePath_)) {
        errorMessage_ = QStringLiteral("Choose a different destination from the original document.");
        emit stateChanged();
        return false;
    }

    const QString text = continuousTextContent_.isEmpty()
        ? textContent_
        : continuousTextContent_;
    if (text.isEmpty()) {
        errorMessage_ = QStringLiteral("This document does not contain selectable text to export.");
        emit stateChanged();
        return false;
    }

    QSaveFile file(normalizedPath);
    const QByteArray data = text.toUtf8();
    if (!file.open(QIODevice::WriteOnly)
        || file.write(data) != data.size()
        || !file.commit()) {
        errorMessage_ = QStringLiteral("The text export could not be written.");
        emit stateChanged();
        return false;
    }
    errorMessage_.clear();
    emit stateChanged();
    return true;
}

bool ReaderController::exportPdf(const QString &path)
{
    if (!open_ || documentEngine_ == nullptr) {
        return false;
    }
    const QString normalizedPath = path.trimmed();
    const QFileInfo outputInfo(normalizedPath);
    if (normalizedPath.isEmpty() || !outputInfo.absoluteDir().exists()) {
        errorMessage_ = QStringLiteral("Choose a valid destination folder for the PDF export.");
        emit stateChanged();
        return false;
    }
    if (sameFilePath(normalizedPath, filePath_)) {
        errorMessage_ = QStringLiteral("Choose a different destination from the original document.");
        emit stateChanged();
        return false;
    }

    QPdfWriter writer(normalizedPath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(144);
    writer.setTitle(title_);
    if (!renderDocument(writer) || !QFileInfo::exists(normalizedPath)
        || QFileInfo(normalizedPath).size() <= 0) {
        QFile::remove(normalizedPath);
        errorMessage_ = QStringLiteral("The PDF export could not be written.");
        emit stateChanged();
        return false;
    }
    errorMessage_.clear();
    emit stateChanged();
    return true;
}

bool ReaderController::printDocument()
{
    if (!open_ || documentEngine_ == nullptr) {
        return false;
    }

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer);
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }
    if (!renderDocument(printer)) {
        errorMessage_ = QStringLiteral("The document could not be sent to the printer.");
        emit stateChanged();
        return false;
    }
    errorMessage_.clear();
    emit stateChanged();
    return true;
}

bool ReaderController::renderDocument(QPagedPaintDevice &device) const
{
    if (!open_ || documentEngine_ == nullptr || pageCount_ <= 0) {
        return false;
    }

    QPainter painter;
    if (!painter.begin(&device)) {
        return false;
    }
    const QRect pageRect = device.pageLayout().paintRectPixels(device.logicalDpiX());
    if (!pageRect.isValid() || pageRect.isEmpty()) {
        painter.end();
        return false;
    }
    const QVariantList savedHighlights = library_ == nullptr
        ? QVariantList{}
        : library_->highlightsForDocument(documentId_);
    const QVariantList savedAnnotations = library_ == nullptr
        ? QVariantList{}
        : library_->annotationsForDocument(documentId_);

    for (int page = 0; page < pageCount_; ++page) {
        if (page > 0 && !device.newPage()) {
            painter.end();
            return false;
        }

        Lumina::Domain::Error error;
        if (documentEngine_->pageKind(page) == Lumina::Domain::Reader::PageKind::Image) {
            QImage image = documentEngine_->renderPage(page, pageRect.size(), &error);
            if (!image.isNull() && pageRotation_ != 0) {
                image = image.transformed(QTransform().rotate(pageRotation_), Qt::SmoothTransformation);
            }
            if (image.isNull()) {
                painter.end();
                return false;
            }
            const QSize scaledSize = image.size().scaled(pageRect.size(), Qt::KeepAspectRatio);
            const QPoint topLeft = pageRect.center()
                - QPoint(scaledSize.width() / 2, scaledSize.height() / 2);
            painter.drawImage(QRect(topLeft, scaledSize), image);
            for (const QVariant &value : savedAnnotations) {
                const QVariantMap annotation = value.toMap();
                if (annotation.value(QStringLiteral("kind")).toString().toLower()
                        != QStringLiteral("region")
                    || annotation.value(QStringLiteral("page")).toInt() != page) {
                    continue;
                }
                const qreal x = std::clamp(annotation.value(QStringLiteral("x")).toDouble(), 0.0, 1.0);
                const qreal y = std::clamp(annotation.value(QStringLiteral("y")).toDouble(), 0.0, 1.0);
                const qreal width = std::clamp(annotation.value(QStringLiteral("width")).toDouble(), 0.0, 1.0 - x);
                const qreal height = std::clamp(annotation.value(QStringLiteral("height")).toDouble(), 0.0, 1.0 - y);
                if (width <= 0.0 || height <= 0.0) {
                    continue;
                }
                QRectF region(QPointF(topLeft) + QPointF(x * scaledSize.width(), y * scaledSize.height()),
                              QSizeF(width * scaledSize.width(), height * scaledSize.height()));
                QColor color(QStringLiteral("#f6c945"));
                color.setAlpha(78);
                painter.save();
                painter.setPen(QPen(QColor(QStringLiteral("#a47700")),
                                    std::max<qreal>(1.0, pageRect.width() / 1800.0)));
                painter.setBrush(color);
                painter.drawRect(region);
                const QString note = annotation.value(QStringLiteral("text")).toString().trimmed();
                if (!note.isEmpty() && region.width() >= 80.0 && region.height() >= 24.0) {
                    painter.setPen(QColor(QStringLiteral("#2b2200")));
                    painter.drawText(region.adjusted(4.0, 4.0, -4.0, -4.0),
                                     Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                                     note.left(240));
                }
                painter.restore();
            }
            for (const QVariant &highlightValue : savedHighlights) {
                const QVariantMap highlight = highlightValue.toMap();
                if (highlight.value(QStringLiteral("page")).toInt() != page) {
                    continue;
                }
                const QStringList highlightParts = highlight.value(QStringLiteral("locator"))
                                                       .toString()
                                                       .split(QChar(':'), Qt::KeepEmptyParts);
                if (highlightParts.size() < 4
                    || highlightParts.first() != QStringLiteral("text")) {
                    continue;
                }
                bool highlightStartOk = false;
                bool highlightEndOk = false;
                const int highlightStart = highlightParts.at(highlightParts.size() - 2)
                                               .toInt(&highlightStartOk);
                const int highlightEnd = highlightParts.last().toInt(&highlightEndOk);
                if (!highlightStartOk || !highlightEndOk || highlightEnd <= highlightStart) {
                    continue;
                }
                for (const QVariant &wordValue : savedAnnotations) {
                    const QVariantMap word = wordValue.toMap();
                    if (word.value(QStringLiteral("kind")).toString().toLower()
                            != QStringLiteral("ocr-word")
                        || word.value(QStringLiteral("page")).toInt() != page) {
                        continue;
                    }
                    const QStringList wordParts = word.value(QStringLiteral("locator"))
                                                      .toString()
                                                      .split(QChar(':'), Qt::KeepEmptyParts);
                    if (wordParts.size() < 4 || wordParts.first() != QStringLiteral("ocr")) {
                        continue;
                    }
                    bool wordStartOk = false;
                    bool wordEndOk = false;
                    const int wordStart = wordParts.at(wordParts.size() - 2).toInt(&wordStartOk);
                    const int wordEnd = wordParts.last().toInt(&wordEndOk);
                    if (!wordStartOk || !wordEndOk || wordEnd <= highlightStart || wordStart >= highlightEnd) {
                        continue;
                    }
                    const qreal x = std::clamp(word.value(QStringLiteral("x")).toDouble(), 0.0, 1.0);
                    const qreal y = std::clamp(word.value(QStringLiteral("y")).toDouble(), 0.0, 1.0);
                    const qreal width = std::clamp(word.value(QStringLiteral("width")).toDouble(), 0.0, 1.0 - x);
                    const qreal height = std::clamp(word.value(QStringLiteral("height")).toDouble(), 0.0, 1.0 - y);
                    if (width <= 0.0 || height <= 0.0) {
                        continue;
                    }
                    QRectF region(QPointF(topLeft) + QPointF(x * scaledSize.width(), y * scaledSize.height()),
                                  QSizeF(width * scaledSize.width(), height * scaledSize.height()));
                    QColor color(highlight.value(QStringLiteral("color")).toString());
                    if (!color.isValid()) {
                        color = QColor(QStringLiteral("#f6c945"));
                    }
                    const QString style = highlight.value(QStringLiteral("style")).toString().toLower();
                    color.setAlpha(style == QStringLiteral("highlight") ? 96 : 42);
                    painter.save();
                    painter.setPen(QPen(color.darker(140),
                                        std::max<qreal>(1.0, pageRect.width() / 2200.0)));
                    painter.setBrush(color);
                    painter.drawRect(region);
                    painter.restore();
                }
            }
            continue;
        }

        const QString text = documentEngine_->textPage(page, &error);
        if (error.hasError()) {
            painter.end();
            return false;
        }
        QTextDocument document;
        document.setDocumentMargin(0);
        if (QGuiApplication::instance() != nullptr) {
            document.setDefaultFont(QFont(QStringLiteral("Segoe UI"), 11));
        }
        document.setPlainText(text);
        document.setTextWidth(pageRect.width());
        const qreal documentHeight = std::max<qreal>(document.size().height(), 1.0);
        const qreal scale = std::min<qreal>(1.0, pageRect.height() / documentHeight);
        QAbstractTextDocumentLayout::PaintContext paintContext;
        for (const QVariant &value : savedHighlights) {
            const QVariantMap highlight = value.toMap();
            if (highlight.value(QStringLiteral("page")).toInt() != page) {
                continue;
            }
            const QStringList locatorParts = highlight.value(QStringLiteral("locator"))
                                                 .toString()
                                                 .split(QChar(':'), Qt::KeepEmptyParts);
            if (locatorParts.size() < 3 || locatorParts.first() != QStringLiteral("text")) {
                continue;
            }
            bool startOk = false;
            bool endOk = false;
            const int start = locatorParts.at(locatorParts.size() - (locatorParts.size() >= 4 ? 2 : 1))
                                  .toInt(&startOk);
            const int end = locatorParts.size() >= 4
                ? locatorParts.last().toInt(&endOk)
                : start + highlight.value(QStringLiteral("text")).toString().size();
            if (!startOk || (locatorParts.size() >= 4 && !endOk)) {
                continue;
            }
            const int maximumPosition = std::max(0, document.characterCount() - 1);
            const int boundedStart = std::clamp(start, 0, maximumPosition);
            const int boundedEnd = std::clamp(std::max(end, boundedStart), boundedStart, maximumPosition);
            if (boundedStart == boundedEnd) {
                continue;
            }
            QTextCursor cursor(&document);
            cursor.setPosition(boundedStart);
            cursor.setPosition(boundedEnd, QTextCursor::KeepAnchor);
            QAbstractTextDocumentLayout::Selection selection;
            selection.cursor = cursor;
            QColor color(highlight.value(QStringLiteral("color")).toString());
            if (!color.isValid()) {
                color = QColor(QStringLiteral("#f6c945"));
            }
            color.setAlpha(96);
            selection.format.setBackground(color);
            const QString style = highlight.value(QStringLiteral("style")).toString().toLower();
            if (style == QStringLiteral("underline")) {
                selection.format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
                selection.format.setUnderlineColor(color.darker(140));
            } else if (style == QStringLiteral("strike")) {
                selection.format.setFontStrikeOut(true);
            }
            paintContext.selections.append(selection);
        }
        painter.save();
        painter.translate(pageRect.topLeft());
        painter.scale(scale, scale);
        paintContext.clip = QRectF(0,
                                   0,
                                   pageRect.width() / scale,
                                   pageRect.height() / scale);
        document.documentLayout()->draw(&painter, paintContext);
        painter.restore();
    }
    return painter.end();
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
    pageImageCache_.clear();
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
    if (ocrDocumentRunning_ || ocrProvider_->busy()) {
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
        [this](const Lumina::Domain::OcrProvider::Result &result, const QString &error) {
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
                                       result.text,
                                       QStringLiteral("completed"),
                                       ocrWordsAsVariants(result))) {
                ocrStatus_ = QStringLiteral("The recognized text could not be saved.");
                emit stateChanged();
                return;
            }
            if (targetDocumentId == documentId_ && targetPage == currentPage_) {
                textContent_ = result.text;
                showOcrText_ = !result.text.isEmpty();
                rebuildSearchMatches();
            }
            ocrStatus_ = result.text.isEmpty()
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

bool ReaderController::startDocumentOcr(const QString &language)
{
    if (!open_ || documentEngine_ == nullptr || ocrProvider_ == nullptr || ocrRunning()) {
        return false;
    }

    QList<int> imagePages;
    for (int page = 0; page < pageCount_; ++page) {
        if (documentEngine_->pageKind(page) != Lumina::Domain::Reader::PageKind::Image) {
            continue;
        }
        Lumina::Domain::Error error;
        const QString nativeText = documentEngine_->textPage(page, &error);
        if (!error.hasError() && !nativeText.trimmed().isEmpty()) {
            continue;
        }
        imagePages.append(page);
    }
    if (imagePages.isEmpty()) {
        ocrStatus_ = QStringLiteral("This document has no image pages that need OCR.");
        emit stateChanged();
        return false;
    }
    if (!ocrProvider_->isAvailable()) {
        ocrStatus_ = QStringLiteral(
            "Tesseract OCR is not installed. Install it and make it available on PATH.");
        emit stateChanged();
        return false;
    }

    ocrBatchDocumentId_ = documentId_;
    ocrBatchLanguage_ = language.trimmed().isEmpty() ? QStringLiteral("eng") : language.trimmed();
    ocrBatchPages_ = std::move(imagePages);
    ocrBatchIndex_ = 0;
    ocrBatchSucceeded_ = 0;
    ocrBatchFailed_ = 0;
    ocrDocumentRunning_ = true;
    ocrPage_ = -1;
    ocrStatus_ = QStringLiteral("Preparing OCR for %1 image pages…").arg(ocrBatchPages_.size());
    emit stateChanged();
    processNextDocumentOcrPage();
    return true;
}

void ReaderController::processNextDocumentOcrPage()
{
    if (!ocrDocumentRunning_ || ocrProvider_ == nullptr || library_ == nullptr) {
        return;
    }
    if (ocrBatchIndex_ >= ocrBatchPages_.size()) {
        const int total = ocrBatchPages_.size();
        ocrDocumentRunning_ = false;
        ocrBatchDocumentId_.clear();
        ocrBatchLanguage_.clear();
        ocrPage_ = -1;
        ocrStatus_ = ocrBatchFailed_ == 0
            ? QStringLiteral("OCR completed for %1 image pages.").arg(ocrBatchSucceeded_)
            : QStringLiteral("OCR completed for %1 of %2 image pages; %3 could not be processed.")
                  .arg(ocrBatchSucceeded_)
                  .arg(total)
                  .arg(ocrBatchFailed_);
        emit stateChanged();
        return;
    }

    const int targetPage = ocrBatchPages_.at(ocrBatchIndex_);
    const QImage image = pageImage(targetPage);
    if (image.isNull()) {
        ++ocrBatchFailed_;
        ++ocrBatchIndex_;
        ocrStatus_ = QStringLiteral("Could not render OCR page %1 of %2.")
                         .arg(targetPage + 1)
                         .arg(ocrBatchPages_.size());
        emit stateChanged();
        QTimer::singleShot(0, this, [this]() { processNextDocumentOcrPage(); });
        return;
    }

    ocrPage_ = targetPage;
    ocrStatus_ = QStringLiteral("Recognizing page %1 of %2…")
                     .arg(targetPage + 1)
                     .arg(ocrBatchPages_.size());
    const QString targetDocumentId = ocrBatchDocumentId_;
    const QString targetLanguage = ocrBatchLanguage_;
    const int initialBatchIndex = ocrBatchIndex_;
    const bool started = ocrProvider_->recognize(
        image,
        targetLanguage,
        [this, targetDocumentId, targetLanguage, targetPage](
            const Lumina::Domain::OcrProvider::Result &result,
                                                              const QString &error) {
            if (!ocrDocumentRunning_ || targetDocumentId != ocrBatchDocumentId_
                || library_ == nullptr) {
                return;
            }
            if (error.isEmpty()
                && library_->saveOcrPage(targetDocumentId,
                                         targetPage,
                                         targetLanguage,
                                         result.text,
                                         QStringLiteral("completed"),
                                         ocrWordsAsVariants(result))) {
                ++ocrBatchSucceeded_;
            } else {
                ++ocrBatchFailed_;
            }
            ++ocrBatchIndex_;
            ocrPage_ = -1;
            emit stateChanged();
            QTimer::singleShot(0, this, [this]() { processNextDocumentOcrPage(); });
        });
    if (!started && ocrBatchIndex_ == initialBatchIndex) {
        ++ocrBatchFailed_;
        ++ocrBatchIndex_;
        ocrPage_ = -1;
        QTimer::singleShot(0, this, [this]() { processNextDocumentOcrPage(); });
    }
}

void ReaderController::cancelOcr()
{
    if (!ocrRunning()) {
        return;
    }
    ocrDocumentRunning_ = false;
    ocrBatchDocumentId_.clear();
    ocrBatchLanguage_.clear();
    ocrBatchPages_.clear();
    ocrBatchIndex_ = 0;
    ocrBatchSucceeded_ = 0;
    ocrBatchFailed_ = 0;
    ocrDocumentId_.clear();
    ocrLanguage_.clear();
    ocrPage_ = -1;
    if (ocrProvider_ != nullptr && ocrProvider_->busy()) {
        ocrProvider_->cancel();
    }
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
        if (searchStarts_.isEmpty()) {
            return false;
        }
        // Refreshing the page rebuilds the matches and intentionally clears the
        // selected match. Keep the same query usable after page navigation by
        // selecting its first match again instead of returning a stale
        // success with searchCurrent() still at -1.
        if (searchCurrent_ < 0) {
            const bool found = selectSearchMatch(0);
            emit stateChanged();
            return found;
        }
        return true;
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

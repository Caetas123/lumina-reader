#include "infrastructure/reader/DocumentEngineFactory.h"

#include "infrastructure/archive/ArchiveReader.h"
#include "infrastructure/text/TextDecoder.h"

#include <QFileInfo>
#include <QImageReader>

#ifdef LUMINA_HAS_QT_PDF
#include <QPdfDocument>
#endif

#include <algorithm>
#include <utility>

namespace {

using Lumina::Domain::Error;
using Lumina::Domain::ErrorCode;
using Lumina::Domain::Reader::DocumentEngine;
using Lumina::Domain::Reader::PageKind;

constexpr int kTextCharactersPerPage = 5000;

void setError(Error *error, ErrorCode code, const QString &message)
{
    if (error != nullptr) {
        *error = Error::create(code, message);
    }
}

class ImageEngine final : public DocumentEngine {
public:
    QString name() const override { return QStringLiteral("Image engine"); }

    bool open(const QString &filePath, Error *error) override
    {
        close();
        QImageReader reader(filePath);
        if (!reader.canRead() || !image_.load(filePath)) {
            setError(error,
                     ErrorCode::Unsupported,
                     QStringLiteral("The image could not be opened. It may be damaged or unsupported."));
            return false;
        }
        filePath_ = filePath;
        return true;
    }

    void close() override
    {
        image_ = {};
        filePath_.clear();
    }

    int pageCount() const override { return image_.isNull() ? 0 : 1; }

    PageKind pageKind(int page) const override
    {
        return page == 0 && !image_.isNull() ? PageKind::Image : PageKind::Text;
    }

    QString pageName(int page) const override
    {
        return page == 0 ? filePath_ : QString();
    }

    QString textPage(int page, Error *error) const override
    {
        if (page != 0 || image_.isNull()) {
            setError(error, ErrorCode::InvalidArgument, QStringLiteral("The image page is invalid."));
        }
        return {};
    }

    QImage renderPage(int page, const QSize &targetSize, Error *error) const override
    {
        if (page != 0 || image_.isNull()) {
            setError(error, ErrorCode::InvalidArgument, QStringLiteral("The image page is invalid."));
            return {};
        }
        if (!targetSize.isValid() || targetSize.isEmpty()) {
            return image_;
        }
        return image_.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

private:
    QString filePath_;
    QImage image_;
};

class TextEngine final : public DocumentEngine {
public:
    explicit TextEngine(QString format)
        : format_(std::move(format))
    {
    }

    QString name() const override { return QStringLiteral("Text engine"); }

    bool open(const QString &filePath, Error *error) override
    {
        close();
        const QString decoded = Lumina::Infrastructure::TextDecoder::readFile(filePath, format_);
        if (decoded.isEmpty() && QFileInfo(filePath).size() > 0) {
            setError(error,
                     ErrorCode::Unsupported,
                     QStringLiteral("The text document could not be decoded."));
            return false;
        }
        filePath_ = filePath;
        text_ = decoded;
        pageCount_ = static_cast<int>(std::max<qsizetype>(
            1, (text_.size() + kTextCharactersPerPage - 1) / kTextCharactersPerPage));
        return true;
    }

    void close() override
    {
        filePath_.clear();
        text_.clear();
        pageCount_ = 0;
    }

    int pageCount() const override { return pageCount_; }

    PageKind pageKind(int page) const override
    {
        return page >= 0 && page < pageCount_ ? PageKind::Text : PageKind::Text;
    }

    QString pageName(int page) const override
    {
        return page >= 0 && page < pageCount_ ? filePath_ : QString();
    }

    QString textPage(int page, Error *error) const override
    {
        if (page < 0 || page >= pageCount_) {
            setError(error, ErrorCode::InvalidArgument, QStringLiteral("The text page is invalid."));
            return {};
        }
        return text_.mid(static_cast<qsizetype>(page) * kTextCharactersPerPage,
                         kTextCharactersPerPage);
    }

    QImage renderPage(int page, const QSize &, Error *error) const override
    {
        if (page < 0 || page >= pageCount_) {
            setError(error, ErrorCode::InvalidArgument, QStringLiteral("The text page is invalid."));
        }
        return {};
    }

private:
    QString format_;
    QString filePath_;
    QString text_;
    int pageCount_ = 0;
};

class ArchiveEngine final : public DocumentEngine {
public:
    explicit ArchiveEngine(QString format)
        : format_(std::move(format))
        , archive_(std::make_unique<Lumina::Infrastructure::ArchiveReader>())
    {
    }

    QString name() const override { return QStringLiteral("Archive engine"); }

    bool open(const QString &filePath, Error *error) override
    {
        close();
        if (!archive_->open(filePath, format_)) {
            setError(error,
                     ErrorCode::Unsupported,
                     QStringLiteral("This archive could not be opened. It may be damaged or unsupported."));
            return false;
        }
        filePath_ = filePath;
        return true;
    }

    void close() override
    {
        archive_ = std::make_unique<Lumina::Infrastructure::ArchiveReader>();
        filePath_.clear();
    }

    int pageCount() const override { return archive_->pageCount(); }

    PageKind pageKind(int page) const override
    {
        return archive_->pageIsImage(page) ? PageKind::Image : PageKind::Text;
    }

    QString pageName(int page) const override { return archive_->pageName(page); }

    QString textPage(int page, Error *error) const override
    {
        if (page < 0 || page >= pageCount()) {
            setError(error, ErrorCode::InvalidArgument, QStringLiteral("The archive page is invalid."));
            return {};
        }
        return archive_->pageText(page);
    }

    QImage renderPage(int page, const QSize &targetSize, Error *error) const override
    {
        if (page < 0 || page >= pageCount() || !archive_->pageIsImage(page)) {
            setError(error, ErrorCode::InvalidArgument, QStringLiteral("The archive image page is invalid."));
            return {};
        }
        const QImage image = QImage::fromData(archive_->pageData(page));
        if (image.isNull()) {
            setError(error, ErrorCode::Unsupported, QStringLiteral("The archive image could not be decoded."));
            return {};
        }
        return targetSize.isValid() && !targetSize.isEmpty()
            ? image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)
            : image;
    }

private:
    QString format_;
    QString filePath_;
    std::unique_ptr<Lumina::Infrastructure::ArchiveReader> archive_;
};

#ifdef LUMINA_HAS_QT_PDF
class PdfEngine final : public DocumentEngine {
public:
    QString name() const override { return QStringLiteral("Qt PDF engine"); }

    bool open(const QString &filePath, Error *error) override
    {
        close();
        document_ = std::make_unique<QPdfDocument>();
        if (document_->load(filePath) != QPdfDocument::Error::None || document_->pageCount() <= 0) {
            close();
            setError(error,
                     ErrorCode::Unsupported,
                     QStringLiteral("The PDF could not be opened. It may be damaged, encrypted, or unsupported."));
            return false;
        }
        filePath_ = filePath;
        return true;
    }

    void close() override
    {
        document_.reset();
        filePath_.clear();
    }

    int pageCount() const override { return document_ == nullptr ? 0 : document_->pageCount(); }

    PageKind pageKind(int page) const override
    {
        return page >= 0 && page < pageCount() ? PageKind::Image : PageKind::Text;
    }

    QString pageName(int page) const override
    {
        return page >= 0 && page < pageCount() ? filePath_ : QString();
    }

    QString textPage(int page, Error *error) const override
    {
        if (document_ == nullptr || page < 0 || page >= pageCount()) {
            setError(error, ErrorCode::InvalidArgument, QStringLiteral("The PDF page is invalid."));
            return {};
        }
        const QPdfSelection selection = document_->getAllText(page);
        return selection.isValid() ? selection.text() : QString();
    }

    QImage renderPage(int page, const QSize &targetSize, Error *error) const override
    {
        if (document_ == nullptr || page < 0 || page >= pageCount()) {
            setError(error, ErrorCode::InvalidArgument, QStringLiteral("The PDF page is invalid."));
            return {};
        }
        const QImage image = document_->render(page, targetSize);
        if (image.isNull()) {
            setError(error, ErrorCode::Unsupported, QStringLiteral("The PDF page could not be rendered."));
        }
        return image;
    }

private:
    QString filePath_;
    std::unique_ptr<QPdfDocument> document_;
};
#endif

} // namespace

namespace Lumina::Infrastructure {

std::unique_ptr<Lumina::Domain::Reader::DocumentEngine>
createDocumentEngine(const QString &format)
{
    const QString normalized = format.trimmed().toLower();
    if (normalized == QStringLiteral("jpg") || normalized == QStringLiteral("jpeg")
        || normalized == QStringLiteral("png") || normalized == QStringLiteral("webp")
        || normalized == QStringLiteral("avif") || normalized == QStringLiteral("gif")) {
        return std::make_unique<ImageEngine>();
    }
    if (normalized == QStringLiteral("txt") || normalized == QStringLiteral("rtf")
        || normalized == QStringLiteral("fb2") || normalized == QStringLiteral("mobi")
        || normalized == QStringLiteral("azw") || normalized == QStringLiteral("azw3")) {
        return std::make_unique<TextEngine>(normalized);
    }
    if (normalized == QStringLiteral("epub") || normalized == QStringLiteral("cbz")
        || normalized == QStringLiteral("zip")
#ifdef LUMINA_HAS_LIBARCHIVE
        || normalized == QStringLiteral("cbr") || normalized == QStringLiteral("cb7")
        || normalized == QStringLiteral("cbt") || normalized == QStringLiteral("rar")
        || normalized == QStringLiteral("7z")
#endif
    ) {
        return std::make_unique<ArchiveEngine>(normalized);
    }
#ifdef LUMINA_HAS_QT_PDF
    if (normalized == QStringLiteral("pdf")) {
        return std::make_unique<PdfEngine>();
    }
#else
    Q_UNUSED(normalized)
#endif
    return nullptr;
}

} // namespace Lumina::Infrastructure

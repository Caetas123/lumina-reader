#include "infrastructure/reader/DjvuEngine.h"

#include <QFileInfo>

#ifdef LUMINA_HAS_DJVU
#if __has_include(<libdjvu/ddjvuapi.h>)
#include <libdjvu/ddjvuapi.h>
#include <libdjvu/miniexp.h>
#else
#include <ddjvuapi.h>
#include <miniexp.h>
#endif
#endif

#include <algorithm>

namespace {

void setError(Lumina::Domain::Error *error,
              Lumina::Domain::ErrorCode code,
              const QString &message)
{
    if (error != nullptr) {
        *error = Lumina::Domain::Error::create(code, message);
    }
}

} // namespace

namespace Lumina::Infrastructure {

struct DjvuEngine::State {
#ifdef LUMINA_HAS_DJVU
    ddjvu_context_t *context = nullptr;
    ddjvu_document_t *document = nullptr;
#endif
};

DjvuEngine::DjvuEngine()
    : state_(std::make_unique<State>())
{
}

DjvuEngine::~DjvuEngine()
{
    close();
}

QString DjvuEngine::name() const
{
    return QStringLiteral("DjVuLibre engine");
}

bool DjvuEngine::open(const QString &filePath, Lumina::Domain::Error *error)
{
    close();

#ifndef LUMINA_HAS_DJVU
    Q_UNUSED(filePath)
    setError(error,
             Lumina::Domain::ErrorCode::Unsupported,
             QStringLiteral("DjVu support is not available in this build. Install DjVuLibre and rebuild Lumina Reader."));
    return false;
#else
    if (!QFileInfo::exists(filePath) || !QFileInfo(filePath).isFile()) {
        setError(error,
                 Lumina::Domain::ErrorCode::FileSystem,
                 QStringLiteral("The DjVu file could not be opened."));
        return false;
    }

    state_->context = ddjvu_context_create("Lumina Reader");
    if (state_->context == nullptr) {
        setError(error,
                 Lumina::Domain::ErrorCode::Unsupported,
                 QStringLiteral("DjVuLibre could not create a decoder context."));
        return false;
    }
    ddjvu_cache_set_size(state_->context, 64UL * 1024UL * 1024UL);

    const QByteArray encodedPath = filePath.toUtf8();
#if defined(DDJVUAPI_VERSION) && DDJVUAPI_VERSION >= 19
    state_->document = ddjvu_document_create_by_filename_utf8(
        state_->context, encodedPath.constData(), TRUE);
#else
    state_->document = ddjvu_document_create_by_filename(
        state_->context, QFile::encodeName(filePath).constData(), TRUE);
#endif
    if (state_->document == nullptr || !waitForDocument(error)
        || ddjvu_document_get_pagenum(state_->document) <= 0) {
        if (error == nullptr || !error->hasError()) {
            setError(error,
                     Lumina::Domain::ErrorCode::Unsupported,
                     QStringLiteral("The DjVu document could not be decoded."));
        }
        close();
        return false;
    }

    filePath_ = filePath;
    return true;
#endif
}

void DjvuEngine::close()
{
#ifdef LUMINA_HAS_DJVU
    if (state_ != nullptr && state_->document != nullptr) {
        ddjvu_document_release(state_->document);
        state_->document = nullptr;
    }
    if (state_ != nullptr && state_->context != nullptr) {
        ddjvu_context_release(state_->context);
        state_->context = nullptr;
    }
#endif
    filePath_.clear();
}

int DjvuEngine::pageCount() const
{
#ifdef LUMINA_HAS_DJVU
    return state_ != nullptr && state_->document != nullptr
        ? ddjvu_document_get_pagenum(state_->document)
        : 0;
#else
    return 0;
#endif
}

Lumina::Domain::Reader::PageKind DjvuEngine::pageKind(int page) const
{
    return page >= 0 && page < pageCount()
        ? Lumina::Domain::Reader::PageKind::Image
        : Lumina::Domain::Reader::PageKind::Text;
}

QString DjvuEngine::pageName(int page) const
{
    return page >= 0 && page < pageCount() ? filePath_ : QString();
}

QString DjvuEngine::textPage(int page, Lumina::Domain::Error *error) const
{
#ifndef LUMINA_HAS_DJVU
    Q_UNUSED(page)
    setError(error,
             Lumina::Domain::ErrorCode::Unsupported,
             QStringLiteral("DjVu text extraction is not available in this build."));
    return {};
#else
    if (state_ == nullptr || state_->document == nullptr || page < 0 || page >= pageCount()) {
        setError(error,
                 Lumina::Domain::ErrorCode::InvalidArgument,
                 QStringLiteral("The DjVu page is invalid."));
        return {};
    }

    miniexp_t expression = ddjvu_document_get_pagetext(state_->document, page, "page");
    while (expression == miniexp_dummy) {
        if (ddjvu_message_wait(state_->context) == nullptr) {
            setError(error,
                     Lumina::Domain::ErrorCode::Unsupported,
                     QStringLiteral("The DjVu page text could not be decoded."));
            return {};
        }
        bool messageError = false;
        while (ddjvu_message_t *message = ddjvu_message_peek(state_->context)) {
            messageError = messageError || message->m_any.tag == DDJVU_ERROR;
            ddjvu_message_pop(state_->context);
        }
        if (messageError) {
            setError(error,
                     Lumina::Domain::ErrorCode::Unsupported,
                     QStringLiteral("The DjVu page text could not be decoded."));
            return {};
        }
        expression = ddjvu_document_get_pagetext(state_->document, page, "page");
    }

    QString result;
    const miniexp_t text = miniexp_nth(5, expression);
    if (miniexp_stringp(text)) {
        result = QString::fromUtf8(miniexp_to_str(text));
    }
    ddjvu_miniexp_release(state_->document, expression);
    return result;
#endif
}

QImage DjvuEngine::renderPage(int page,
                              const QSize &targetSize,
                              Lumina::Domain::Error *error) const
{
#ifndef LUMINA_HAS_DJVU
    Q_UNUSED(page)
    Q_UNUSED(targetSize)
    setError(error,
             Lumina::Domain::ErrorCode::Unsupported,
             QStringLiteral("DjVu rendering is not available in this build."));
    return {};
#else
    if (state_ == nullptr || state_->document == nullptr || page < 0 || page >= pageCount()) {
        setError(error,
                 Lumina::Domain::ErrorCode::InvalidArgument,
                 QStringLiteral("The DjVu page is invalid."));
        return {};
    }

    ddjvu_page_t *decodedPage = ddjvu_page_create_by_pageno(state_->document, page);
    if (decodedPage == nullptr || !waitForPage(decodedPage, error)) {
        if (decodedPage != nullptr) {
            ddjvu_page_release(decodedPage);
        }
        return {};
    }

    const int sourceWidth = ddjvu_page_get_width(decodedPage);
    const int sourceHeight = ddjvu_page_get_height(decodedPage);
    if (sourceWidth <= 0 || sourceHeight <= 0) {
        ddjvu_page_release(decodedPage);
        setError(error,
                 Lumina::Domain::ErrorCode::Unsupported,
                 QStringLiteral("The DjVu page has no usable dimensions."));
        return {};
    }

    int width = sourceWidth;
    int height = sourceHeight;
    if (targetSize.isValid() && !targetSize.isEmpty()) {
        const QSize scaled = QSize(sourceWidth, sourceHeight).scaled(
            targetSize, Qt::KeepAspectRatio);
        width = std::max(1, scaled.width());
        height = std::max(1, scaled.height());
    }
    if (width > 20000 || height > 20000
        || static_cast<qint64>(width) * static_cast<qint64>(height) > 160000000) {
        ddjvu_page_release(decodedPage);
        setError(error,
                 Lumina::Domain::ErrorCode::Unsupported,
                 QStringLiteral("The DjVu page is too large to render safely."));
        return {};
    }

    QImage image(width, height, QImage::Format_RGB888);
    if (image.isNull()) {
        ddjvu_page_release(decodedPage);
        setError(error,
                 Lumina::Domain::ErrorCode::FileSystem,
                 QStringLiteral("The DjVu page could not be allocated."));
        return {};
    }

    ddjvu_rect_t pageRect{0, 0, width, height};
    ddjvu_rect_t renderRect{0, 0, width, height};
    ddjvu_format_t *format = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, nullptr);
    if (format == nullptr) {
        ddjvu_page_release(decodedPage);
        setError(error,
                 Lumina::Domain::ErrorCode::Unsupported,
                 QStringLiteral("DjVuLibre could not create an image format."));
        return {};
    }
    ddjvu_format_set_row_order(format, TRUE);
    const int success = ddjvu_page_render(decodedPage,
                                          DDJVU_RENDER_COLOR,
                                          &pageRect,
                                          &renderRect,
                                          format,
                                          static_cast<unsigned long>(image.bytesPerLine()),
                                          reinterpret_cast<char *>(image.bits()));
    ddjvu_format_release(format);
    ddjvu_page_release(decodedPage);
    if (!success) {
        setError(error,
                 Lumina::Domain::ErrorCode::Unsupported,
                 QStringLiteral("The DjVu page could not be rendered."));
        return {};
    }
    return image;
#endif
}

bool DjvuEngine::waitForDocument(Lumina::Domain::Error *error) const
{
#ifndef LUMINA_HAS_DJVU
    Q_UNUSED(error)
    return false;
#else
    if (state_ == nullptr || state_->context == nullptr || state_->document == nullptr) {
        return false;
    }
    while (!ddjvu_document_decoding_done(state_->document)) {
        if (ddjvu_message_wait(state_->context) == nullptr) {
            break;
        }
        while (ddjvu_message_t *message = ddjvu_message_peek(state_->context)) {
            if (message->m_any.tag == DDJVU_ERROR && error != nullptr) {
                setError(error,
                         Lumina::Domain::ErrorCode::Unsupported,
                         QStringLiteral("The DjVu document could not be decoded."));
            }
            ddjvu_message_pop(state_->context);
        }
    }
    return !ddjvu_document_decoding_error(state_->document);
#endif
}

bool DjvuEngine::waitForPage(void *page, Lumina::Domain::Error *error) const
{
#ifndef LUMINA_HAS_DJVU
    Q_UNUSED(page)
    Q_UNUSED(error)
    return false;
#else
    auto *decodedPage = static_cast<ddjvu_page_t *>(page);
    if (state_ == nullptr || state_->context == nullptr || decodedPage == nullptr) {
        return false;
    }
    while (!ddjvu_page_decoding_done(decodedPage)) {
        if (ddjvu_message_wait(state_->context) == nullptr) {
            break;
        }
        while (ddjvu_message_t *message = ddjvu_message_peek(state_->context)) {
            if (message->m_any.tag == DDJVU_ERROR && error != nullptr) {
                setError(error,
                         Lumina::Domain::ErrorCode::Unsupported,
                         QStringLiteral("The DjVu page could not be decoded."));
            }
            ddjvu_message_pop(state_->context);
        }
    }
    return !ddjvu_page_decoding_error(decodedPage);
#endif
}

} // namespace Lumina::Infrastructure

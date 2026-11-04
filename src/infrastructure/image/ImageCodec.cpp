#include "infrastructure/image/ImageCodec.h"

#include <QFile>
#include <QFileInfo>
#include <QImageReader>

#include <algorithm>
#include <cstring>

#ifdef LUMINA_HAS_LIBWEBP
#include <webp/decode.h>
#endif

#ifdef LUMINA_HAS_LIBAVIF
#include <avif/avif.h>
#endif

namespace {

constexpr qint64 kMaximumImageBytes = 256LL * 1024LL * 1024LL;

QByteArray readImageBytes(const QString &filePath)
{
    const QFileInfo info(filePath);
    if (!info.isFile() || info.size() <= 0 || info.size() > kMaximumImageBytes) {
        return {};
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

#ifdef LUMINA_HAS_LIBWEBP
QImage loadWebp(const QByteArray &bytes)
{
    int width = 0;
    int height = 0;
    if (bytes.isEmpty()
        || !WebPGetInfo(reinterpret_cast<const uint8_t *>(bytes.constData()), bytes.size(), &width, &height)
        || width <= 0 || height <= 0 || width > 20000 || height > 20000) {
        return {};
    }

    uint8_t *decoded = WebPDecodeRGBA(
        reinterpret_cast<const uint8_t *>(bytes.constData()), bytes.size(), &width, &height);
    if (decoded == nullptr) {
        return {};
    }

    QImage image(width, height, QImage::Format_RGBA8888);
    for (int y = 0; y < height; ++y) {
        std::memcpy(image.scanLine(y), decoded + (static_cast<qsizetype>(y) * width * 4),
                     static_cast<size_t>(image.bytesPerLine()));
    }
    WebPFree(decoded);
    return image;
}
#endif

#ifdef LUMINA_HAS_LIBAVIF
QImage loadAvif(const QByteArray &bytes)
{
    if (bytes.isEmpty()) {
        return {};
    }

    avifDecoder *decoder = avifDecoderCreate();
    if (decoder == nullptr) {
        return {};
    }

    QImage image;
    const auto *data = reinterpret_cast<const uint8_t *>(bytes.constData());
    const avifResult ioResult = avifDecoderSetIOMemory(decoder, data, static_cast<size_t>(bytes.size()));
    const avifResult parseResult = ioResult == AVIF_RESULT_OK ? avifDecoderParse(decoder) : ioResult;
    const avifResult nextResult = parseResult == AVIF_RESULT_OK ? avifDecoderNextImage(decoder) : parseResult;
    if (nextResult == AVIF_RESULT_OK
        && decoder->image != nullptr
        && decoder->image->width > 0 && decoder->image->height > 0
        && decoder->image->width <= 20000 && decoder->image->height <= 20000) {
        avifRGBImage rgb;
        avifRGBImageSetDefaults(&rgb, decoder->image);
        rgb.format = AVIF_RGB_FORMAT_RGBA;
        rgb.depth = 8;
        if (avifRGBImageAllocatePixels(&rgb) == AVIF_RESULT_OK
            && avifImageYUVToRGB(decoder->image, &rgb) == AVIF_RESULT_OK) {
            image = QImage(static_cast<int>(rgb.width), static_cast<int>(rgb.height),
                           QImage::Format_RGBA8888);
            for (uint32_t y = 0; y < rgb.height; ++y) {
                std::memcpy(image.scanLine(static_cast<int>(y)),
                            rgb.pixels + (static_cast<size_t>(y) * rgb.rowBytes),
                            static_cast<size_t>(image.bytesPerLine()));
            }
        }
        avifRGBImageFreePixels(&rgb);
    }

    avifDecoderDestroy(decoder);
    return image;
}
#endif

} // namespace

namespace Lumina::Infrastructure {

QImage loadImageFile(const QString &filePath)
{
    QImageReader reader(filePath);
    QImage image;
    if (reader.canRead() && reader.read(&image) && !image.isNull()) {
        return image;
    }

    const QString suffix = QFileInfo(filePath).suffix().toLower();
    const QByteArray bytes = readImageBytes(filePath);
#ifdef LUMINA_HAS_LIBWEBP
    if (suffix == QStringLiteral("webp")) {
        return loadWebp(bytes);
    }
#else
    Q_UNUSED(bytes)
#endif
#ifdef LUMINA_HAS_LIBAVIF
    if (suffix == QStringLiteral("avif")) {
        return loadAvif(bytes);
    }
#endif
    return {};
}

bool canReadImageFile(const QString &filePath)
{
    return !loadImageFile(filePath).isNull();
}

} // namespace Lumina::Infrastructure

#pragma once

#include "domain/core/Error.h"

#include <QImage>
#include <QString>
#include <QSize>
#include <QSizeF>

#include <memory>

namespace Lumina::Pdf {

class PdfDocument {
public:
    virtual ~PdfDocument() = default;

    [[nodiscard]] virtual int pageCount() const = 0;
    [[nodiscard]] virtual QSizeF pageSize(int pageIndex) const = 0;
    [[nodiscard]] virtual QImage renderPage(int pageIndex,
                                            qreal scale,
                                            const QSize &targetSize,
                                            Domain::Error *error = nullptr) = 0;
};

class PdfEngine {
public:
    virtual ~PdfEngine() = default;

    [[nodiscard]] virtual QString name() const = 0;
    [[nodiscard]] virtual std::unique_ptr<PdfDocument> open(
        const QString &filePath,
        Domain::Error *error = nullptr) = 0;
};

} // namespace Lumina::Pdf

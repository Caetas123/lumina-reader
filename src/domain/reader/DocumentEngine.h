#pragma once

#include "domain/core/Error.h"

#include <QImage>
#include <QString>
#include <QSize>

namespace Lumina::Domain::Reader {

enum class PageKind {
    Text,
    Image,
};

class DocumentEngine {
public:
    virtual ~DocumentEngine() = default;

    [[nodiscard]] virtual QString name() const = 0;
    [[nodiscard]] virtual bool open(const QString &filePath, Domain::Error *error = nullptr) = 0;
    virtual void close() = 0;
    [[nodiscard]] virtual int pageCount() const = 0;
    [[nodiscard]] virtual PageKind pageKind(int page) const = 0;
    [[nodiscard]] virtual QString pageName(int page) const = 0;
    [[nodiscard]] virtual QString textPage(int page, Domain::Error *error = nullptr) const = 0;
    [[nodiscard]] virtual QImage renderPage(int page,
                                            const QSize &targetSize,
                                            Domain::Error *error = nullptr) const = 0;
};

} // namespace Lumina::Domain::Reader

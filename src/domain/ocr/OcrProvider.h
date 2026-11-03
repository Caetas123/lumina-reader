#pragma once

#include <QImage>
#include <QString>

#include <functional>

namespace Lumina::Domain {

class OcrProvider {
public:
    using Completion = std::function<void(const QString &text, const QString &error)>;

    virtual ~OcrProvider() = default;

    [[nodiscard]] virtual QString name() const = 0;
    [[nodiscard]] virtual bool isAvailable() const = 0;
    [[nodiscard]] virtual bool recognize(const QImage &image,
                                         const QString &language,
                                         Completion completion) = 0;
    virtual void cancel() = 0;
};

} // namespace Lumina::Domain

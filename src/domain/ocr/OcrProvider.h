#pragma once

#include <QImage>
#include <QList>
#include <QString>

#include <functional>

namespace Lumina::Domain {

class OcrProvider {
public:
    struct Word {
        QString text;
        int start = 0;
        int length = 0;
        double x = 0.0;
        double y = 0.0;
        double width = 0.0;
        double height = 0.0;
    };

    struct Result {
        QString text;
        QList<Word> words;
    };

    using Completion = std::function<void(const Result &result, const QString &error)>;

    virtual ~OcrProvider() = default;

    [[nodiscard]] virtual QString name() const = 0;
    [[nodiscard]] virtual bool isAvailable() const = 0;
    [[nodiscard]] virtual bool recognize(const QImage &image,
                                         const QString &language,
                                         Completion completion) = 0;
    virtual void cancel() = 0;
};

} // namespace Lumina::Domain

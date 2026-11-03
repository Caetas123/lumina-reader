#pragma once

#include "domain/reader/DocumentEngine.h"

#include <QString>

#include <memory>

namespace Lumina::Infrastructure {

class DjvuEngine final : public Lumina::Domain::Reader::DocumentEngine {
public:
    DjvuEngine();
    ~DjvuEngine() override;

    [[nodiscard]] QString name() const override;
    [[nodiscard]] bool open(const QString &filePath,
                            Lumina::Domain::Error *error = nullptr) override;
    void close() override;
    [[nodiscard]] int pageCount() const override;
    [[nodiscard]] Lumina::Domain::Reader::PageKind pageKind(int page) const override;
    [[nodiscard]] QString pageName(int page) const override;
    [[nodiscard]] QString textPage(int page,
                                   Lumina::Domain::Error *error = nullptr) const override;
    [[nodiscard]] QImage renderPage(int page,
                                    const QSize &targetSize,
                                    Lumina::Domain::Error *error = nullptr) const override;

private:
    struct State;

    [[nodiscard]] bool waitForDocument(Lumina::Domain::Error *error) const;
    [[nodiscard]] bool waitForPage(void *page, Lumina::Domain::Error *error) const;

    std::unique_ptr<State> state_;
    QString filePath_;
};

} // namespace Lumina::Infrastructure

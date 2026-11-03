#pragma once

#include <QString>

namespace Lumina::Infrastructure {

class TextDecoder final {
public:
    [[nodiscard]] static QString readFile(const QString &path, const QString &format);
};

} // namespace Lumina::Infrastructure

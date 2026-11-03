#pragma once

#include "domain/reader/DocumentEngine.h"

#include <QString>

#include <memory>

namespace Lumina::Infrastructure {

[[nodiscard]] std::unique_ptr<Lumina::Domain::Reader::DocumentEngine>
createDocumentEngine(const QString &format);

} // namespace Lumina::Infrastructure

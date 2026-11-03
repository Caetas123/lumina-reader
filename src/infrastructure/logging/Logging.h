#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(luminaApp)
Q_DECLARE_LOGGING_CATEGORY(luminaDatabase)
Q_DECLARE_LOGGING_CATEGORY(luminaPdf)

namespace Lumina::Infrastructure {

void initializeLogging();

} // namespace Lumina::Infrastructure

#include "infrastructure/logging/Logging.h"

#include <QCoreApplication>

Q_LOGGING_CATEGORY(luminaApp, "lumina.app")
Q_LOGGING_CATEGORY(luminaDatabase, "lumina.database")
Q_LOGGING_CATEGORY(luminaPdf, "lumina.pdf")

namespace Lumina::Infrastructure {

void initializeLogging()
{
    qSetMessagePattern(QStringLiteral("[%{time yyyy-MM-dd HH:mm:ss.zzz}] %{type} %{category}: %{message}"));
    qCInfo(luminaApp) << QCoreApplication::applicationName()
                      << QCoreApplication::applicationVersion()
                      << "starting";
}

} // namespace Lumina::Infrastructure

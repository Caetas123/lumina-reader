#include "application/AppController.h"

namespace Lumina::Application {

AppController::AppController(QObject *parent)
    : QObject(parent)
{
}

QString AppController::applicationName() const
{
    return QStringLiteral("Lumina Reader");
}

QString AppController::applicationVersion() const
{
    return QStringLiteral(LUMINA_VERSION);
}

} // namespace Lumina::Application

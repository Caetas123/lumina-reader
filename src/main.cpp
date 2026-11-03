#include "application/AppController.h"

#include "infrastructure/logging/Logging.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Lumina Reader"));
    QCoreApplication::setApplicationVersion(QStringLiteral(LUMINA_VERSION));
    QCoreApplication::setOrganizationName(QStringLiteral("Lumina Reader contributors"));
    Lumina::Infrastructure::initializeLogging();

    QQmlApplicationEngine engine;
    Lumina::Application::AppController appController;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &appController);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &application,
        [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("LuminaReader"), QStringLiteral("Main"));

    return application.exec();
}

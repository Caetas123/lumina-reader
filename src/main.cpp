#include "application/AppController.h"

#include "infrastructure/logging/Logging.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>

int main(int argc, char *argv[])
{
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")) {
        qputenv("QT_QUICK_CONTROLS_STYLE", QByteArrayLiteral("Basic"));
    }
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Lumina Reader"));
    QCoreApplication::setApplicationVersion(QStringLiteral(LUMINA_VERSION));
    QCoreApplication::setOrganizationName(QStringLiteral("Lumina Reader contributors"));
    application.setWindowIcon(QIcon(QStringLiteral(":/qt/qml/LuminaReader/assets/lumina-logo.png")));
    Lumina::Infrastructure::initializeLogging();

    QQmlApplicationEngine engine;
    Lumina::Application::AppController appController;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &appController);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::warnings,
        &application,
        [](const QList<QQmlError> &warnings) {
            for (const QQmlError &warning : warnings) {
                qCWarning(luminaApp) << warning.toString();
            }
        });

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &application,
        [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("LuminaReader"), QStringLiteral("Main"));

    return application.exec();
}

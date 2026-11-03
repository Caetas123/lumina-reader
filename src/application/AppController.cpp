#include "application/AppController.h"
#include "application/ReaderController.h"
#include "application/SettingsController.h"

#include "domain/core/Error.h"
#include "infrastructure/database/Database.h"
#include "infrastructure/library/LibraryStore.h"
#include "infrastructure/logging/Logging.h"

#include <QDir>
#include <QStandardPaths>

#include <memory>

namespace Lumina::Application {

AppController::AppController(QObject *parent)
    : QObject(parent)
    , database_(std::make_unique<Lumina::Infrastructure::Database>())
    , library_(std::make_unique<Lumina::Infrastructure::LibraryStore>())
    , reader_(std::make_unique<ReaderController>(library_.get()))
    , settings_(std::make_unique<SettingsController>())
{
    initializeDatabase();
}

AppController::~AppController() = default;

void AppController::initializeDatabase()
{
    databaseReady_ = false;
    startupError_.clear();

    const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataPath.isEmpty()) {
        const auto error = Lumina::Domain::Error::create(
            Lumina::Domain::ErrorCode::FileSystem,
            QStringLiteral("The application data directory is unavailable."));
        startupError_ = error.userMessage();
        qCWarning(luminaApp) << error.logMessage();
        emit databaseStateChanged();
        return;
    }

    QDir appDataDirectory;
    if (!appDataDirectory.mkpath(appDataPath)) {
        const auto error = Lumina::Domain::Error::create(
            Lumina::Domain::ErrorCode::FileSystem,
            QStringLiteral("The application could not create its data directory."));
        startupError_ = error.userMessage();
        qCWarning(luminaApp) << error.logMessage();
        emit databaseStateChanged();
        return;
    }

    const QString databasePath = QDir(appDataPath).filePath(QStringLiteral("lumina.db"));
    Lumina::Domain::Error error;
    if (!database_->open(databasePath, &error) || !database_->migrate(&error)) {
        startupError_ = error.userMessage();
        qCWarning(luminaApp) << error.logMessage();
        emit databaseStateChanged();
        return;
    }

    databaseReady_ = true;
    library_->setDatabase(database_.get());
    settings_->setDatabase(database_.get());
    qCInfo(luminaApp) << "Local database initialized";
    emit databaseStateChanged();
}

QString AppController::applicationName() const
{
    return QStringLiteral("Lumina Reader");
}

QString AppController::applicationVersion() const
{
    return QStringLiteral(LUMINA_VERSION);
}

bool AppController::databaseReady() const
{
    return databaseReady_;
}

QString AppController::startupError() const
{
    return startupError_;
}

Lumina::Infrastructure::LibraryStore *AppController::library() const
{
    return library_.get();
}

QObject *AppController::reader() const
{
    return reader_.get();
}

QObject *AppController::settings() const
{
    return settings_.get();
}

void AppController::retryInitialization()
{
    initializeDatabase();
}

} // namespace Lumina::Application

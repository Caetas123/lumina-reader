#pragma once

#include "infrastructure/library/LibraryStore.h"

#include <QObject>
#include <QString>

#include <memory>

namespace Lumina::Infrastructure {
class Database;
}

namespace Lumina::Application {

class AppController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString applicationName READ applicationName CONSTANT)
    Q_PROPERTY(QString applicationVersion READ applicationVersion CONSTANT)
    Q_PROPERTY(bool databaseReady READ databaseReady NOTIFY databaseStateChanged)
    Q_PROPERTY(QString startupError READ startupError NOTIFY databaseStateChanged)
    Q_PROPERTY(Lumina::Infrastructure::LibraryStore *library READ library CONSTANT)

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    [[nodiscard]] QString applicationName() const;
    [[nodiscard]] QString applicationVersion() const;
    [[nodiscard]] bool databaseReady() const;
    [[nodiscard]] QString startupError() const;
    [[nodiscard]] Lumina::Infrastructure::LibraryStore *library() const;

    Q_INVOKABLE void retryInitialization();

signals:
    void databaseStateChanged();

private:
    void initializeDatabase();

    std::unique_ptr<Lumina::Infrastructure::Database> database_;
    std::unique_ptr<Lumina::Infrastructure::LibraryStore> library_;
    bool databaseReady_ = false;
    QString startupError_;
};

} // namespace Lumina::Application

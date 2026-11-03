#pragma once

#include <QObject>
#include <QString>

namespace Lumina::Infrastructure {
class Database;
}

namespace Lumina::Application {

class BackupController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY stateChanged)

public:
    explicit BackupController(QObject *parent = nullptr);

    [[nodiscard]] QString lastError() const;
    void setDatabase(Lumina::Infrastructure::Database *database);

    Q_INVOKABLE bool exportKnowledge(const QString &path, const QString &format);
    Q_INVOKABLE bool exportLibrary(const QString &path);
    Q_INVOKABLE bool restoreLibrary(const QString &path);
    Q_INVOKABLE bool backupDatabase(const QString &path);

signals:
    void stateChanged();

private:
    bool writeFile(const QString &path, const QByteArray &data);
    bool validatePath(const QString &path);
    bool validateSourcePath(const QString &path);
    void fail(const QString &message);

    Lumina::Infrastructure::Database *database_ = nullptr;
    QString lastError_;
};

} // namespace Lumina::Application

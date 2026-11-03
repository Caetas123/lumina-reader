#pragma once

#include "domain/core/Error.h"

#include <QSqlDatabase>

namespace Lumina::Infrastructure {

class Database final {
public:
    Database();
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    [[nodiscard]] bool open(const QString &databasePath, Domain::Error *error = nullptr);
    [[nodiscard]] bool migrate(Domain::Error *error = nullptr);
    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] int schemaVersion() const;
    [[nodiscard]] QSqlDatabase connection() const;

private:
    QString connectionName_;
    QSqlDatabase connection_;
};

} // namespace Lumina::Infrastructure

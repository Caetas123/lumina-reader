#pragma once

#include <QObject>
#include <QString>

namespace Lumina::Application {

class AppController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString applicationName READ applicationName CONSTANT)
    Q_PROPERTY(QString applicationVersion READ applicationVersion CONSTANT)

public:
    explicit AppController(QObject *parent = nullptr);

    [[nodiscard]] QString applicationName() const;
    [[nodiscard]] QString applicationVersion() const;
};

} // namespace Lumina::Application

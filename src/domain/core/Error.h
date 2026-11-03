#pragma once

#include <QString>

#include <utility>

namespace Lumina::Domain {

enum class ErrorCode {
    None,
    InvalidArgument,
    FileSystem,
    Database,
    Migration,
    Unsupported,
};

class Error final {
public:
    Error() = default;

    static Error create(ErrorCode code, QString userMessage, QString technicalMessage = {})
    {
        Error error;
        error.code_ = code;
        error.userMessage_ = std::move(userMessage);
        error.technicalMessage_ = technicalMessage.isEmpty() ? error.userMessage_ : std::move(technicalMessage);
        return error;
    }

    [[nodiscard]] bool hasError() const { return code_ != ErrorCode::None; }
    [[nodiscard]] ErrorCode code() const { return code_; }
    [[nodiscard]] const QString &userMessage() const { return userMessage_; }
    [[nodiscard]] const QString &logMessage() const { return technicalMessage_; }

private:
    ErrorCode code_ = ErrorCode::None;
    QString userMessage_;
    QString technicalMessage_;
};

} // namespace Lumina::Domain

#pragma once

#include <QString>

namespace pyraqt::core {

enum class UpdateStatus {
    NotConfigured,
    UpToDate,
    UpdateAvailable,
    Failed,
};

struct UpdateCheckResult {
    UpdateStatus status = UpdateStatus::NotConfigured;
    QString message;
    QString latestVersion;
    QString downloadUrl;
};

} // namespace pyraqt::core

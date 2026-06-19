#pragma once

#include "core/update/update_types.h"

#include <QObject>
#include <QString>

namespace pyraqt::core {

class ConfigManager;
class LogManager;

class UpdateManager final : public QObject {
    Q_OBJECT

public:
    UpdateManager(ConfigManager &configManager, LogManager &logManager, QObject *parent = nullptr);

    [[nodiscard]] QString currentChannel() const;
    void setChannel(const QString &channel);
    [[nodiscard]] QString lastCheckTime() const;
    [[nodiscard]] bool autoCheckEnabled() const;
    void setAutoCheckEnabled(bool enabled);

public slots:
    UpdateCheckResult checkForUpdates(bool manual);

signals:
    void updateCheckStarted();
    void updateCheckFinished(const pyraqt::core::UpdateCheckResult &result);
    void updateStatusChanged(const QString &message);

private:
    ConfigManager &m_configManager;
    LogManager &m_logManager;
};

} // namespace pyraqt::core

Q_DECLARE_METATYPE(pyraqt::core::UpdateCheckResult)

#pragma once

#include <QObject>
#include <QString>

namespace pyraqt::core {

class ConfigManager;
class LogManager;

class CrashRecoveryManager final : public QObject {
    Q_OBJECT

public:
    CrashRecoveryManager(ConfigManager &configManager, LogManager &logManager, QObject *parent = nullptr);

    void markAppStarted();
    void markAppExitedNormally();
    [[nodiscard]] bool didPreviousRunCrash() const;
    [[nodiscard]] QString crashLogPath() const;
    [[nodiscard]] QString recoveryMessage() const;

private:
    [[nodiscard]] QString runtimeDirectory() const;
    void appendCrashLog(const QString &message) const;

    ConfigManager &m_configManager;
    LogManager &m_logManager;
    bool m_previousRunCrashed = false;
};

} // namespace pyraqt::core

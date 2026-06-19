#pragma once

#include <QObject>
#include <QString>

namespace pyraqt::core {

class ConfigManager;

class PythonRuntimeManager final : public QObject {
    Q_OBJECT

public:
    explicit PythonRuntimeManager(ConfigManager &configManager, QObject *parent = nullptr);

    [[nodiscard]] QString interpreterPath() const;
    void setInterpreterPath(const QString &path);
    void setExecutionTimeoutMs(int timeoutMs);

    [[nodiscard]] bool isInterpreterAvailable() const;
    [[nodiscard]] QString pythonVersion() const;
    [[nodiscard]] int executionTimeoutMs() const;

signals:
    void interpreterChanged(const QString &path);
    void executionTimeoutChanged(int timeoutMs);

private:
    ConfigManager &m_configManager;
};

} // namespace pyraqt::core

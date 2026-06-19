#include "core/scripting/python_runtime_manager.h"

#include "core/config/config_manager.h"

#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace pyraqt::core {

PythonRuntimeManager::PythonRuntimeManager(ConfigManager &configManager, QObject *parent)
    : QObject(parent)
    , m_configManager(configManager)
{
}

QString PythonRuntimeManager::interpreterPath() const
{
    return m_configManager.value(QStringLiteral("python.interpreter_path"), QStringLiteral("python3")).toString();
}

void PythonRuntimeManager::setInterpreterPath(const QString &path)
{
    m_configManager.setValue(QStringLiteral("python.interpreter_path"), path);
    const bool saved = m_configManager.save();
    Q_UNUSED(saved)
    emit interpreterChanged(path);
}

void PythonRuntimeManager::setExecutionTimeoutMs(int timeoutMs)
{
    m_configManager.setValue(QStringLiteral("python.execution_timeout_ms"), timeoutMs);
    const bool saved = m_configManager.save();
    Q_UNUSED(saved)
    emit executionTimeoutChanged(timeoutMs);
}

bool PythonRuntimeManager::isInterpreterAvailable() const
{
    const QString path = interpreterPath();
    if (QFileInfo::exists(path) && QFileInfo(path).isExecutable()) {
        return true;
    }
    return !QStandardPaths::findExecutable(path).isEmpty();
}

QString PythonRuntimeManager::pythonVersion() const
{
    QProcess process;
    process.start(interpreterPath(), {QStringLiteral("--version")});
    if (!process.waitForFinished(3000)) {
        return QStringLiteral("Unavailable");
    }

    const QString stdoutOutput = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString stderrOutput = QString::fromUtf8(process.readAllStandardError()).trimmed();
    const QString version = !stdoutOutput.isEmpty() ? stdoutOutput : stderrOutput;
    return version.isEmpty() ? QStringLiteral("Unavailable") : version;
}

int PythonRuntimeManager::executionTimeoutMs() const
{
    return m_configManager.value(QStringLiteral("python.execution_timeout_ms"), 15000).toInt();
}

} // namespace pyraqt::core

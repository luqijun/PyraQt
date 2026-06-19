#include "core/runtime/crash_recovery_manager.h"

#include "core/config/config_manager.h"
#include "core/logging/log_manager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QTextStream>

namespace pyraqt::core {
namespace {

QString dataRootPath()
{
    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (environment.contains(QStringLiteral("PYRAQT_DATA_DIR"))) {
        return environment.value(QStringLiteral("PYRAQT_DATA_DIR"));
    }
    return QDir::temp().filePath(QStringLiteral("PyraQt"));
}

} // namespace

CrashRecoveryManager::CrashRecoveryManager(ConfigManager &configManager, LogManager &logManager, QObject *parent)
    : QObject(parent)
    , m_configManager(configManager)
    , m_logManager(logManager)
{
}

void CrashRecoveryManager::markAppStarted()
{
    m_previousRunCrashed = !m_configManager.value(QStringLiteral("runtime.last_exit_clean"), true).toBool();
    m_configManager.setValue(QStringLiteral("runtime.last_exit_clean"), false);
    m_configManager.setValue(QStringLiteral("runtime.last_start"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!m_configManager.save()) {
        m_logManager.warning(QStringLiteral("Failed to persist runtime start marker"));
    }

    if (m_previousRunCrashed) {
        appendCrashLog(QStringLiteral("Detected previous abnormal exit."));
        m_logManager.warning(recoveryMessage());
    }
}

void CrashRecoveryManager::markAppExitedNormally()
{
    m_configManager.setValue(QStringLiteral("runtime.last_exit_clean"), true);
    m_configManager.setValue(QStringLiteral("runtime.last_exit"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!m_configManager.save()) {
        m_logManager.warning(QStringLiteral("Failed to persist runtime exit marker"));
    }
}

bool CrashRecoveryManager::didPreviousRunCrash() const
{
    return m_previousRunCrashed;
}

QString CrashRecoveryManager::crashLogPath() const
{
    return QDir(runtimeDirectory()).filePath(QStringLiteral("crash.log"));
}

QString CrashRecoveryManager::recoveryMessage() const
{
    return QStringLiteral("The previous PyraQt session may not have exited cleanly.");
}

QString CrashRecoveryManager::runtimeDirectory() const
{
    const QString path = QDir(dataRootPath()).filePath(QStringLiteral("crash"));
    QDir dir(path);
    dir.mkpath(QStringLiteral("."));
    return dir.absolutePath();
}

void CrashRecoveryManager::appendCrashLog(const QString &message) const
{
    QFile file(crashLogPath());
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);
    stream << QDateTime::currentDateTimeUtc().toString(Qt::ISODate) << " " << message << '\n';
}

} // namespace pyraqt::core

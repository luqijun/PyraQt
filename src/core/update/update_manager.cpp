#include "core/update/update_manager.h"

#include "core/config/config_manager.h"
#include "core/logging/log_manager.h"

#include <QDateTime>

namespace pyraqt::core {

UpdateManager::UpdateManager(ConfigManager &configManager, LogManager &logManager, QObject *parent)
    : QObject(parent)
    , m_configManager(configManager)
    , m_logManager(logManager)
{
    qRegisterMetaType<pyraqt::core::UpdateCheckResult>("pyraqt::core::UpdateCheckResult");
}

QString UpdateManager::currentChannel() const
{
    return m_configManager.value(QStringLiteral("updates.channel"), QStringLiteral("stable")).toString();
}

void UpdateManager::setChannel(const QString &channel)
{
    m_configManager.setValue(QStringLiteral("updates.channel"), channel);
    const bool saved = m_configManager.save();
    if (!saved) {
        m_logManager.warning(QStringLiteral("Failed to persist update channel"));
    }
}

QString UpdateManager::lastCheckTime() const
{
    return m_configManager.value(QStringLiteral("updates.last_check")).toString();
}

bool UpdateManager::autoCheckEnabled() const
{
    return m_configManager.value(QStringLiteral("updates.auto_check"), false).toBool();
}

void UpdateManager::setAutoCheckEnabled(bool enabled)
{
    m_configManager.setValue(QStringLiteral("updates.auto_check"), enabled);
    const bool saved = m_configManager.save();
    if (!saved) {
        m_logManager.warning(QStringLiteral("Failed to persist update auto-check setting"));
    }
}

UpdateCheckResult UpdateManager::checkForUpdates(bool manual)
{
    Q_UNUSED(manual)

    emit updateCheckStarted();

    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    m_configManager.setValue(QStringLiteral("updates.last_check"), now);
    const bool saved = m_configManager.save();
    if (!saved) {
        m_logManager.warning(QStringLiteral("Failed to persist update check timestamp"));
    }

    UpdateCheckResult result;
    result.status = UpdateStatus::NotConfigured;
    result.message = QStringLiteral("Update checking is not configured for this development build.");
    result.latestVersion = QStringLiteral("0.1.0");
    result.downloadUrl = m_configManager.value(QStringLiteral("updates.feed_url")).toString();

    m_logManager.info(QStringLiteral("Update check completed: %1").arg(result.message));
    emit updateStatusChanged(result.message);
    emit updateCheckFinished(result);
    return result;
}

} // namespace pyraqt::core

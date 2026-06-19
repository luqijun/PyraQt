#include "core/workspace/workspace_manager.h"

#include "core/config/config_manager.h"

#include <QDir>
#include <QFileInfo>

namespace pyraqt::core {
namespace {

QString cleanedPath(const QString &path)
{
    return QFileInfo(path).exists() ? QFileInfo(path).canonicalFilePath() : QString();
}

} // namespace

WorkspaceManager::WorkspaceManager(ConfigManager &configManager, QObject *parent)
    : QObject(parent)
    , m_configManager(configManager)
{
}

WorkspaceSession WorkspaceManager::restoreSession() const
{
    WorkspaceSession session;
    session.openFiles = sanitizedFiles(m_configManager.value(QStringLiteral("workspace.open_files")).toStringList());
    session.activeFile = cleanedPath(m_configManager.value(QStringLiteral("workspace.active_file")).toString());
    if (!session.openFiles.contains(session.activeFile)) {
        session.activeFile.clear();
    }
    session.recentFiles = sanitizedFiles(m_configManager.value(QStringLiteral("workspace.recent_files")).toStringList());
    session.fileBrowserRoot = m_configManager.value(QStringLiteral("workspace.file_browser_root")).toString();
    if (session.fileBrowserRoot.isEmpty()) {
        session.fileBrowserRoot = m_configManager.value(QStringLiteral("python.last_directory")).toString();
    }
    if (session.fileBrowserRoot.isEmpty()) {
        session.fileBrowserRoot = QDir::homePath();
    }
    return session;
}

void WorkspaceManager::saveSession(const WorkspaceSession &session)
{
    persistStringList(QStringLiteral("workspace.open_files"), sanitizedFiles(session.openFiles));
    m_configManager.setValue(QStringLiteral("workspace.active_file"), cleanedPath(session.activeFile));
    persistStringList(QStringLiteral("workspace.recent_files"), sanitizedFiles(session.recentFiles));
    setFileBrowserRoot(session.fileBrowserRoot);
    const bool saved = m_configManager.save();
    Q_UNUSED(saved)
    emit sessionChanged();
}

QStringList WorkspaceManager::recentFiles() const
{
    return sanitizedFiles(m_configManager.value(QStringLiteral("workspace.recent_files")).toStringList());
}

void WorkspaceManager::addRecentFile(const QString &path)
{
    const QString normalized = cleanedPath(path);
    if (normalized.isEmpty()) {
        return;
    }

    QStringList files = recentFiles();
    files.removeAll(normalized);
    files.prepend(normalized);
    while (files.size() > maxRecentFiles()) {
        files.removeLast();
    }
    persistStringList(QStringLiteral("workspace.recent_files"), files);
    const bool saved = m_configManager.save();
    Q_UNUSED(saved)
    emit recentFilesChanged(files);
}

void WorkspaceManager::clearRecentFiles()
{
    persistStringList(QStringLiteral("workspace.recent_files"), {});
    const bool saved = m_configManager.save();
    Q_UNUSED(saved)
    emit recentFilesChanged({});
}

QString WorkspaceManager::fileBrowserRoot() const
{
    WorkspaceSession session = restoreSession();
    return session.fileBrowserRoot;
}

void WorkspaceManager::setFileBrowserRoot(const QString &path)
{
    QString root = path;
    if (root.isEmpty() || !QFileInfo(root).exists()) {
        root = QDir::homePath();
    }
    m_configManager.setValue(QStringLiteral("workspace.file_browser_root"), QFileInfo(root).absoluteFilePath());
    emit fileBrowserRootChanged(m_configManager.value(QStringLiteral("workspace.file_browser_root")).toString());
}

bool WorkspaceManager::restoreLastSessionEnabled() const
{
    return m_configManager.value(QStringLiteral("workspace.restore_last_session"), true).toBool();
}

void WorkspaceManager::setRestoreLastSessionEnabled(bool enabled)
{
    m_configManager.setValue(QStringLiteral("workspace.restore_last_session"), enabled);
}

QStringList WorkspaceManager::sanitizedFiles(const QStringList &files) const
{
    QStringList result;
    for (const QString &file : files) {
        const QString normalized = cleanedPath(file);
        if (!normalized.isEmpty() && !result.contains(normalized)) {
            result.push_back(normalized);
        }
    }
    return result;
}

int WorkspaceManager::maxRecentFiles() const
{
    return m_configManager.value(QStringLiteral("workspace.max_recent_files"), 10).toInt();
}

void WorkspaceManager::persistStringList(const QString &key, const QStringList &values)
{
    m_configManager.setValue(key, values);
}

} // namespace pyraqt::core

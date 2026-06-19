#pragma once

#include "core/workspace/workspace_types.h"

#include <QObject>
#include <QStringList>

namespace pyraqt::core {

class ConfigManager;

class WorkspaceManager final : public QObject {
    Q_OBJECT

public:
    explicit WorkspaceManager(ConfigManager &configManager, QObject *parent = nullptr);

    [[nodiscard]] WorkspaceSession restoreSession() const;
    void saveSession(const WorkspaceSession &session);

    [[nodiscard]] QStringList recentFiles() const;
    void addRecentFile(const QString &path);
    void clearRecentFiles();

    [[nodiscard]] QString fileBrowserRoot() const;
    void setFileBrowserRoot(const QString &path);

    [[nodiscard]] bool restoreLastSessionEnabled() const;
    void setRestoreLastSessionEnabled(bool enabled);

signals:
    void sessionChanged();
    void recentFilesChanged(const QStringList &files);
    void fileBrowserRootChanged(const QString &path);

private:
    [[nodiscard]] QStringList sanitizedFiles(const QStringList &files) const;
    [[nodiscard]] int maxRecentFiles() const;
    void persistStringList(const QString &key, const QStringList &values);

    ConfigManager &m_configManager;
};

} // namespace pyraqt::core

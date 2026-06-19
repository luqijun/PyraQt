#pragma once

#include "core/plugin/plugin_types.h"

#include <QObject>
#include <QVector>

class QPluginLoader;

namespace pyraqt::core {

class CommandManager;
class ConfigManager;
class IPlugin;
class LogManager;
class PluginContext;
class ScriptExecutionManager;

class PluginManager final : public QObject {
    Q_OBJECT

public:
    PluginManager(
        CommandManager &commandManager,
        ConfigManager &configManager,
        LogManager &logManager,
        ScriptExecutionManager &scriptExecutionManager,
        QObject *parent = nullptr);
    ~PluginManager() override;

    void scanPlugins();
    void loadEnabledPlugins();
    bool setPluginEnabled(const QString &id, bool enabled);
    [[nodiscard]] QVector<PluginInfo> plugins() const;
    [[nodiscard]] QStringList lastErrors() const;

signals:
    void pluginsRefreshed();
    void pluginStateChanged(const QString &pluginId, bool enabled);

private:
    void scanCppPlugins();
    void scanPythonPlugins();
    void loadCppPlugin(const PluginInfo &pluginInfo);
    void loadPythonPlugin(const PluginInfo &pluginInfo);
    void registerPythonPluginCommands(const PluginInfo &pluginInfo);
    [[nodiscard]] bool isPluginEnabled(const QString &id, bool enabledByDefault = true) const;
    void persistDisabledPlugins();
    [[nodiscard]] QStringList pluginSearchPaths() const;
    PluginInfo *findPlugin(const QString &id);

    CommandManager &m_commandManager;
    ConfigManager &m_configManager;
    LogManager &m_logManager;
    ScriptExecutionManager &m_scriptExecutionManager;
    PluginContext *m_pluginContext = nullptr;
    QVector<PluginInfo> m_plugins;
    QStringList m_errors;
    QVector<QPluginLoader *> m_pluginLoaders;
    QStringList m_disabledPluginIds;
};

} // namespace pyraqt::core

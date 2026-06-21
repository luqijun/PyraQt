#pragma once

#include "core/plugin/plugin_types.h"

#include <QFileInfo>
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
class PythonRuntimeManager;

class PluginManager final : public QObject {
    Q_OBJECT

public:
    PluginManager(
        CommandManager &commandManager,
        ConfigManager &configManager,
        LogManager &logManager,
        ScriptExecutionManager &scriptExecutionManager,
        PythonRuntimeManager &pythonRuntimeManager,
        QObject *parent = nullptr);
    ~PluginManager() override;

    void scanPlugins();
    void loadEnabledPlugins();
    bool setPluginEnabled(const QString &id, bool enabled);
    bool reloadPlugin(const QString &id);
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
    void unloadPythonPlugin(const PluginInfo &pluginInfo);
    void registerPythonPluginCommands(const PluginInfo &pluginInfo);
    [[nodiscard]] PluginInfo readPythonPluginInfo(const QFileInfo &entry) const;
    [[nodiscard]] bool isPluginEnabled(const QString &id, bool enabledByDefault = true) const;
    void persistDisabledPlugins();
    [[nodiscard]] QStringList pluginSearchPaths() const;
    PluginInfo *findPlugin(const QString &id);

    CommandManager &m_commandManager;
    ConfigManager &m_configManager;
    LogManager &m_logManager;
    ScriptExecutionManager &m_scriptExecutionManager;
    PythonRuntimeManager &m_pythonRuntimeManager;
    PluginContext *m_pluginContext = nullptr;
    QVector<PluginInfo> m_plugins;
    QStringList m_errors;
    QVector<QPluginLoader *> m_pluginLoaders;
    QStringList m_disabledPluginIds;
};

} // namespace pyraqt::core

#pragma once

#include "core/plugin/plugin_types.h"

#include <QFileInfo>
#include <QHash>
#include <QObject>
#include <QVector>

class QPluginLoader;

namespace pyraqt::core {

class CommandManager;
class ConfigManager;
class LogManager;
class PluginContext;
class PythonFeatureManager;
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
        PythonFeatureManager &pythonFeatureManager,
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
    void unloadAllPlugins();
    void scanCppPlugins();
    void scanPythonPlugins();
    bool loadPlugin(PluginInfo &pluginInfo);
    bool loadCppPlugin(PluginInfo &pluginInfo);
    bool unloadCppPlugin(PluginInfo &pluginInfo);
    bool loadPythonPlugin(PluginInfo &pluginInfo);
    bool unloadPythonPlugin(PluginInfo &pluginInfo);
    void registerPythonPluginCommands(const PluginInfo &pluginInfo);
    [[nodiscard]] PluginInfo readPythonPluginInfo(const QFileInfo &entry) const;
    [[nodiscard]] bool isPluginEnabled(const QString &id, bool enabledByDefault = true) const;
    void persistDisabledPlugins();
    [[nodiscard]] QStringList pluginSearchPaths() const;
    [[nodiscard]] QStringList pluginRootPaths() const;
    [[nodiscard]] QString pythonEntryModuleName(const PluginInfo &pluginInfo) const;
    [[nodiscard]] QString pythonRootModuleName(const PluginInfo &pluginInfo) const;
    [[nodiscard]] bool dependenciesSatisfied(const PluginInfo &pluginInfo, QString *reason = nullptr) const;
    PluginInfo *findPlugin(const QString &id);
    const PluginInfo *findPlugin(const QString &id) const;
    void appendPluginError(PluginInfo &pluginInfo, const QString &message);
    void clearPluginError(PluginInfo &pluginInfo);

    CommandManager &m_commandManager;
    ConfigManager &m_configManager;
    LogManager &m_logManager;
    ScriptExecutionManager &m_scriptExecutionManager;
    PythonFeatureManager &m_pythonFeatureManager;
    PythonRuntimeManager &m_pythonRuntimeManager;
    PluginContext *m_pluginContext = nullptr;
    QVector<PluginInfo> m_plugins;
    QStringList m_errors;
    QHash<QString, QPluginLoader *> m_pluginLoaders;
    QStringList m_disabledPluginIds;
};

} // namespace pyraqt::core

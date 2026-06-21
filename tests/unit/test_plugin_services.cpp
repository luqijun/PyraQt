#include "core/command/command_manager.h"
#include "core/config/config_manager.h"
#include "core/logging/log_manager.h"
#include "core/plugin/plugin_manager.h"
#include "core/scripting/pyra_api_bridge.h"
#include "core/scripting/python_runtime_manager.h"
#include "core/scripting/script_execution_manager.h"

#include <QtTest>

#include <algorithm>

namespace {

class PluginServicesTest final : public QObject {
    Q_OBJECT

private slots:
    void commandRegistration();
    void pluginDiscovery();
    void pythonPluginLifecycleLoadsInProcess();
    void pythonPluginDisableUnloadsInProcess();
    void pythonPluginReloadKeepsPluginActive();
    void disablePluginPersists();
};

void PluginServicesTest::commandRegistration()
{
    pyraqt::core::CommandManager manager;
    pyraqt::core::CommandDescriptor descriptor;
    descriptor.id = QStringLiteral("test.command");
    descriptor.ownerId = QStringLiteral("tests");
    descriptor.title = QStringLiteral("Test Command");
    descriptor.handler = [] {};
    QVERIFY(manager.registerCommand(descriptor));
    QCOMPARE(manager.commands().size(), 1);
    manager.unregisterCommands(QStringLiteral("tests"));
    QCOMPARE(manager.commands().size(), 0);
}

void PluginServicesTest::pluginDiscovery()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, runtimeManager);

    pluginManager.scanPlugins();
    const auto plugins = pluginManager.plugins();
    QVERIFY(!plugins.isEmpty());
}

void PluginServicesTest::pythonPluginLifecycleLoadsInProcess()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, runtimeManager);

    pluginManager.scanPlugins();
    pluginManager.loadEnabledPlugins();
    const auto plugins = pluginManager.plugins();
    QVERIFY(std::any_of(plugins.begin(), plugins.end(), [](const pyraqt::core::PluginInfo &plugin) {
        return plugin.type == QStringLiteral("python") && plugin.loaded && plugin.active;
    }));
}

void PluginServicesTest::pythonPluginReloadKeepsPluginActive()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, runtimeManager);

    pluginManager.scanPlugins();
    pluginManager.loadEnabledPlugins();
    const auto plugins = pluginManager.plugins();
    auto it = std::find_if(plugins.begin(), plugins.end(), [](const pyraqt::core::PluginInfo &plugin) {
        return plugin.type == QStringLiteral("python") && plugin.loaded;
    });
    QVERIFY(it != plugins.end());
    QVERIFY(pluginManager.reloadPlugin(it->id));
    const auto reloadedPlugins = pluginManager.plugins();
    auto reloadedIt = std::find_if(reloadedPlugins.begin(), reloadedPlugins.end(), [it](const pyraqt::core::PluginInfo &plugin) {
        return plugin.id == it->id;
    });
    QVERIFY(reloadedIt != reloadedPlugins.end());
    QVERIFY(reloadedIt->loaded);
    QVERIFY(reloadedIt->active);
}

void PluginServicesTest::pythonPluginDisableUnloadsInProcess()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, runtimeManager);

    pluginManager.scanPlugins();
    pluginManager.loadEnabledPlugins();
    const auto plugins = pluginManager.plugins();
    auto it = std::find_if(plugins.begin(), plugins.end(), [](const pyraqt::core::PluginInfo &plugin) {
        return plugin.type == QStringLiteral("python") && plugin.loaded;
    });
    QVERIFY(it != plugins.end());
    QVERIFY(pluginManager.setPluginEnabled(it->id, false));
    const auto updatedPlugins = pluginManager.plugins();
    auto updatedIt = std::find_if(updatedPlugins.begin(), updatedPlugins.end(), [it](const pyraqt::core::PluginInfo &plugin) {
        return plugin.id == it->id;
    });
    QVERIFY(updatedIt != updatedPlugins.end());
    QVERIFY(!updatedIt->loaded);
    QVERIFY(!updatedIt->active);
}

void PluginServicesTest::disablePluginPersists()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, runtimeManager);

    pluginManager.scanPlugins();
    const auto plugins = pluginManager.plugins();
    QVERIFY(!plugins.isEmpty());
    QVERIFY(pluginManager.setPluginEnabled(plugins.first().id, false));
    QVERIFY(configManager.value(QStringLiteral("plugins.disabled_ids")).toStringList().contains(plugins.first().id));
}

} // namespace

QObject *createPluginServicesTest()
{
    return new PluginServicesTest();
}

#include "test_plugin_services.moc"

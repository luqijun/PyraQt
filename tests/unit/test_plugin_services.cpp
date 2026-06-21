#include "core/command/command_manager.h"
#include "core/config/config_manager.h"
#include "core/logging/log_manager.h"
#include "core/plugin/plugin_manager.h"
#include "core/scripting/pyra_api_bridge.h"
#include "core/scripting/python_feature_manager.h"
#include "core/scripting/python_runner.h"
#include "core/scripting/python_runtime_manager.h"
#include "core/scripting/script_execution_manager.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>

namespace {

class PluginServicesTest final : public QObject {
    Q_OBJECT

private slots:
    void commandRegistration();
    void pluginDiscovery();
    void pluginDiscoveryIncludesConfiguredSearchPaths();
    void pythonPluginLifecycleLoadsInProcess();
    void pythonPluginDisableUnloadsInProcess();
    void pythonPluginReloadKeepsPluginActive();
    void disablePluginPersists();
    void pluginDependencyFailureIsReported();
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
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, featureManager, runtimeManager);

    pluginManager.scanPlugins();
    const auto plugins = pluginManager.plugins();
    QVERIFY(!plugins.isEmpty());
}

void PluginServicesTest::pluginDiscoveryIncludesConfiguredSearchPaths()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString pythonRoot = QDir(dir.path()).filePath(QStringLiteral("python/custom_test_plugin"));
    QVERIFY(QDir().mkpath(pythonRoot));

    QFile manifest(QDir(pythonRoot).filePath(QStringLiteral("plugin.json")));
    QVERIFY(manifest.open(QIODevice::WriteOnly | QIODevice::Text));
    manifest.write(R"({
  "id": "custom_test_plugin",
  "name": "Custom Test Plugin",
  "version": "0.1.0",
  "description": "Discovered from configured path.",
  "entry": "__init__.py"
})");
    manifest.close();

    QFile entry(QDir(pythonRoot).filePath(QStringLiteral("__init__.py")));
    QVERIFY(entry.open(QIODevice::WriteOnly | QIODevice::Text));
    entry.write("def classFactory(iface):\n    return None\n");
    entry.close();

    pyraqt::core::ConfigManager configManager;
    configManager.setValue(QStringLiteral("plugins.search_paths"), QStringList{dir.path()});
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, featureManager, runtimeManager);

    pluginManager.scanPlugins();
    const auto plugins = pluginManager.plugins();
    QVERIFY(std::any_of(plugins.begin(), plugins.end(), [](const pyraqt::core::PluginInfo &plugin) {
        return plugin.id == QStringLiteral("custom_test_plugin");
    }));
}

void PluginServicesTest::pythonPluginLifecycleLoadsInProcess()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, featureManager, runtimeManager);

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
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, featureManager, runtimeManager);

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
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, featureManager, runtimeManager);

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
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, featureManager, runtimeManager);

    pluginManager.scanPlugins();
    const auto plugins = pluginManager.plugins();
    QVERIFY(!plugins.isEmpty());
    QVERIFY(pluginManager.setPluginEnabled(plugins.first().id, false));
    QVERIFY(configManager.value(QStringLiteral("plugins.disabled_ids")).toStringList().contains(plugins.first().id));
}

void PluginServicesTest::pluginDependencyFailureIsReported()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString pluginDir = QDir(dir.path()).filePath(QStringLiteral("python/dependent_test_plugin"));
    QVERIFY(QDir().mkpath(pluginDir));

    QFile manifest(QDir(pluginDir).filePath(QStringLiteral("plugin.json")));
    QVERIFY(manifest.open(QIODevice::WriteOnly | QIODevice::Text));
    manifest.write(R"({
  "id": "dependent_test_plugin",
  "name": "Dependent Test Plugin",
  "version": "0.1.0",
  "description": "Fails when dependency is missing.",
  "entry": "__init__.py",
  "dependencies": ["missing_plugin"]
})");
    manifest.close();

    QFile entry(QDir(pluginDir).filePath(QStringLiteral("__init__.py")));
    QVERIFY(entry.open(QIODevice::WriteOnly | QIODevice::Text));
    entry.write("def classFactory(iface):\n    return None\n");
    entry.close();

    pyraqt::core::ConfigManager configManager;
    configManager.setValue(QStringLiteral("plugins.search_paths"), QStringList{dir.path()});
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);
    pyraqt::core::CommandManager commandManager;
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager, featureManager, runtimeManager);

    pluginManager.scanPlugins();
    pluginManager.loadEnabledPlugins();
    const auto plugins = pluginManager.plugins();
    auto it = std::find_if(plugins.begin(), plugins.end(), [](const pyraqt::core::PluginInfo &plugin) {
        return plugin.id == QStringLiteral("dependent_test_plugin");
    });
    QVERIFY(it != plugins.end());
    QVERIFY(!it->loaded);
    QVERIFY(it->error.contains(QStringLiteral("Missing dependency")));
}

} // namespace

QObject *createPluginServicesTest()
{
    return new PluginServicesTest();
}

#include "test_plugin_services.moc"

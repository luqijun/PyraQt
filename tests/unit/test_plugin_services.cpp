#include "core/command/command_manager.h"
#include "core/config/config_manager.h"
#include "core/logging/log_manager.h"
#include "core/plugin/plugin_manager.h"
#include "core/scripting/pyra_api_bridge.h"
#include "core/scripting/python_runtime_manager.h"
#include "core/scripting/script_execution_manager.h"

#include <QtTest>

namespace {

class PluginServicesTest final : public QObject {
    Q_OBJECT

private slots:
    void commandRegistration();
    void pluginDiscovery();
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
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager);

    pluginManager.scanPlugins();
    const auto plugins = pluginManager.plugins();
    QVERIFY(!plugins.isEmpty());
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
    pyraqt::core::PluginManager pluginManager(commandManager, configManager, logManager, executionManager);

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

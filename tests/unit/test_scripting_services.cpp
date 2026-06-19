#include "core/config/config_manager.h"
#include "core/logging/log_manager.h"
#include "core/scripting/pyra_api_bridge.h"
#include "core/scripting/python_runtime_manager.h"
#include "core/scripting/script_execution_manager.h"

#include <QFileInfo>
#include <QSignalSpy>
#include <QtTest>

namespace {

class ScriptingServicesTest final : public QObject {
    Q_OBJECT

private slots:
    void runtimeDiscovery();
    void bridgeFilesGenerated();
    void runFileEmitsFinish();
};

void ScriptingServicesTest::runtimeDiscovery()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    QVERIFY(runtimeManager.isInterpreterAvailable());
    QVERIFY(runtimeManager.pythonVersion().contains(QStringLiteral("Python")));
}

void ScriptingServicesTest::bridgeFilesGenerated()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);

    QVERIFY(bridge.ensureBridgeFiles());
    QVERIFY(QFileInfo::exists(bridge.bootstrapFilePath()));
}

void ScriptingServicesTest::runFileEmitsFinish()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);

    QSignalSpy finishedSpy(&executionManager, &pyraqt::core::ScriptExecutionManager::executionFinished);
    QVERIFY(executionManager.runFile(QStringLiteral("/home/numbat/Projects/Misc/PyraQt/scripts/hello_pyra.py")));
    QTRY_VERIFY_WITH_TIMEOUT(!finishedSpy.isEmpty(), 10000);
}

} // namespace

QObject *createScriptingServicesTest()
{
    return new ScriptingServicesTest();
}

#include "test_scripting_services.moc"

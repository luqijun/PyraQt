#include "core/config/config_manager.h"
#include "core/logging/log_manager.h"
#include "core/scripting/pyra_api_bridge.h"
#include "core/scripting/python_feature_manager.h"
#include "core/scripting/python_runner.h"
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
    void embeddedRuntimeExecEvalAndStdout();
    void pyraApiBridgeSignals();
    void pyraFilesystemAccessCanBeDisabled();
    void ifaceExposesPyraEntryPoints();
    void pyraCommandRegistrationExecutesCallback();
    void pyraFeatureBridgeSignals();
    void macroExpressionAndProcessingFeatures();
    void processingAsyncEmitsFinished();
    void bridgeFilesGenerated();
    void runCommandReturnsReplResult();
    void runFileEmitsFinish();
};

void ScriptingServicesTest::runtimeDiscovery()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    QVERIFY(runtimeManager.isInterpreterAvailable());
    QVERIFY(runtimeManager.pythonVersion().contains(QStringLiteral("Python")));
}

void ScriptingServicesTest::embeddedRuntimeExecEvalAndStdout()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    QVERIFY(runtimeManager.initializeEmbedded());
    QVERIFY(runtimeManager.isEmbeddedInitialized());

    QSignalSpy stdoutSpy(&runtimeManager, &pyraqt::core::PythonRuntimeManager::stdoutReceived);
    pyraqt::core::PythonRunner runner(runtimeManager);
    QVERIFY(runner.runCode(QStringLiteral("x = 40\nprint('embedded ready')")).success);
    const pyraqt::core::ScriptResult result = runner.eval(QStringLiteral("x + 2"));
    QVERIFY(result.success);
    QCOMPARE(result.resultText, QStringLiteral("42"));
    QVERIFY(!stdoutSpy.isEmpty());
}

void ScriptingServicesTest::pyraApiBridgeSignals()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    QVERIFY(runtimeManager.initializeEmbedded());
    QSignalSpy statusSpy(&runtimeManager, &pyraqt::core::PythonRuntimeManager::statusMessageRequested);
    QSignalSpy logSpy(&runtimeManager, &pyraqt::core::PythonRuntimeManager::logMessageRequested);
    pyraqt::core::PythonRunner runner(runtimeManager);
    QVERIFY(runner.runCode(QStringLiteral("import pyra\npyra.ui.set_status('hello')\npyra.log.info('logged')")).success);
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(logSpy.count(), 1);
}

void ScriptingServicesTest::pyraFilesystemAccessCanBeDisabled()
{
    pyraqt::core::ConfigManager configManager;
    configManager.setValue(QStringLiteral("python.filesystem_access_enabled"), false);
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    QVERIFY(runtimeManager.initializeEmbedded());
    QSignalSpy violationSpy(&runtimeManager, &pyraqt::core::PythonRuntimeManager::filesystemAccessViolation);
    pyraqt::core::PythonRunner runner(runtimeManager);
    const pyraqt::core::ScriptResult result = runner.runCode(QStringLiteral("import pyra\npyra.fs.read_text('/tmp/blocked')"));
    QVERIFY(!result.success);
    QVERIFY(result.traceback.contains(QStringLiteral("PermissionError")));
    QCOMPARE(violationSpy.count(), 1);
}

void ScriptingServicesTest::ifaceExposesPyraEntryPoints()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    QVERIFY(runtimeManager.initializeEmbedded());
    pyraqt::core::PythonRunner runner(runtimeManager);
    QCOMPARE(runner.eval(QStringLiteral("iface.app.name")).resultText, QStringLiteral("PyraQt"));
    QVERIFY(runner.runCode(QStringLiteral("iface.setStatus('via iface')")).success);
    QVERIFY(runner.runCode(QStringLiteral("iface.registerCommand('iface.command', title='Iface Command')")).success);
}

void ScriptingServicesTest::pyraCommandRegistrationExecutesCallback()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    QVERIFY(runtimeManager.initializeEmbedded());
    pyraqt::core::PythonRunner runner(runtimeManager);

    QSignalSpy commandSpy(&runtimeManager, &pyraqt::core::PythonRuntimeManager::commandRegistrationRequested);
    QVERIFY(runner.runCode(QStringLiteral(
                "import pyra\n"
                "def _test_command():\n"
                "    global command_value\n"
                "    command_value = 'ran'\n"
                "pyra.commands.register('test.command', _test_command, title='Test Command')\n"))
                .success);
    QCOMPARE(commandSpy.count(), 1);
    QVERIFY(runner.runCode(QStringLiteral("pyra.commands.run('test.command')")).success);
    QCOMPARE(runner.eval(QStringLiteral("command_value")).resultText, QStringLiteral("ran"));
}

void ScriptingServicesTest::pyraFeatureBridgeSignals()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    QVERIFY(runtimeManager.initializeEmbedded());

    QSignalSpy macroLoadSpy(&runtimeManager, &pyraqt::core::PythonRuntimeManager::macroLoadRequested);
    QSignalSpy macroTriggerSpy(&runtimeManager, &pyraqt::core::PythonRuntimeManager::macroTriggerRequested);
    QSignalSpy expressionSpy(&runtimeManager, &pyraqt::core::PythonRuntimeManager::expressionRegistrationRequested);
    QSignalSpy processingSpy(&runtimeManager, &pyraqt::core::PythonRuntimeManager::processingRegistrationRequested);

    pyraqt::core::PythonRunner runner(runtimeManager);
    QVERIFY(runner.runCode(QStringLiteral(
                "import pyra\n"
                "pyra.macros.load('def openProject():\\n    pass\\n')\n"
                "pyra.macros.trigger('openProject')\n"
                "pyra.expressions.register('identity', 'def identity(value):\\n    return value\\n')\n"
                "pyra.processing.register('alg', 'def alg(params):\\n    return params\\n')\n"))
                .success);
    QCOMPARE(macroLoadSpy.count(), 1);
    QCOMPARE(macroTriggerSpy.count(), 1);
    QCOMPARE(expressionSpy.count(), 1);
    QCOMPARE(processingSpy.count(), 1);
}

void ScriptingServicesTest::macroExpressionAndProcessingFeatures()
{
    pyraqt::core::ConfigManager configManager;
    configManager.setValue(QStringLiteral("python.macros_enabled"), true);
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);

    QVERIFY(featureManager.loadMacros(QStringLiteral("def openProject():\n    global macro_value\n    macro_value = 'opened'\n")));
    QVERIFY(featureManager.triggerMacro(QStringLiteral("openProject")).success);
    QCOMPARE(runner.eval(QStringLiteral("__import__('sys').modules['proj_macros_mod'].macro_value")).resultText, QStringLiteral("opened"));

    QVERIFY(featureManager.registerExpressionFunction(QStringLiteral("double_text"), QStringLiteral("def double_text(value):\n    return value + value\n")));
    QCOMPARE(featureManager.evaluateExpressionFunction(QStringLiteral("double_text"), {QStringLiteral("ab")}).resultText, QStringLiteral("abab"));

    QVERIFY(featureManager.registerProcessingAlgorithm(QStringLiteral("sample_algorithm"), QStringLiteral("def sample_algorithm(params):\n    return params.get('name', '') + '-done'\n")));
    QCOMPARE(featureManager.runProcessingAlgorithm(QStringLiteral("sample_algorithm"), {{QStringLiteral("name"), QStringLiteral("job")}}).resultText, QStringLiteral("job-done"));
}

void ScriptingServicesTest::processingAsyncEmitsFinished()
{
    qRegisterMetaType<pyraqt::core::ScriptResult>("ScriptResult");
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PythonRunner runner(runtimeManager);
    pyraqt::core::PythonFeatureManager featureManager(runtimeManager, runner);
    QVERIFY(featureManager.registerProcessingAlgorithm(QStringLiteral("async_algorithm"), QStringLiteral("def async_algorithm(params):\n    return params.get('name', '') + '-async'\n")));

    QSignalSpy startedSpy(&featureManager, &pyraqt::core::PythonFeatureManager::processingStarted);
    QSignalSpy finishedSpy(&featureManager, &pyraqt::core::PythonFeatureManager::processingFinished);
    featureManager.runProcessingAlgorithmAsync(QStringLiteral("async_algorithm"), {{QStringLiteral("name"), QStringLiteral("job")}});
    QCOMPARE(startedSpy.count(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(!finishedSpy.isEmpty(), 5000);
    const auto arguments = finishedSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QStringLiteral("async_algorithm"));
    const auto result = arguments.at(1).value<pyraqt::core::ScriptResult>();
    QVERIFY(result.success);
    QCOMPARE(result.resultText, QStringLiteral("job-async"));
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

void ScriptingServicesTest::runCommandReturnsReplResult()
{
    pyraqt::core::ConfigManager configManager;
    pyraqt::core::LogManager logManager;
    QVERIFY(logManager.initialize());
    pyraqt::core::PythonRuntimeManager runtimeManager(configManager);
    pyraqt::core::PyraApiBridge bridge(runtimeManager, logManager);
    pyraqt::core::ScriptExecutionManager executionManager(runtimeManager, bridge);

    QVERIFY(executionManager.runCommand(QStringLiteral("repl_value = 42")).success);
    const pyraqt::core::ScriptResult valueResult = executionManager.runCommand(QStringLiteral("repl_value"));
    QVERIFY(valueResult.success);
    QCOMPARE(valueResult.resultText, QStringLiteral("42"));
    const pyraqt::core::ScriptResult noneResult = executionManager.runCommand(QStringLiteral("print('command stdout')"));
    QVERIFY(noneResult.success);
    QVERIFY(noneResult.resultText.isEmpty() || noneResult.resultText == QStringLiteral("None"));
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

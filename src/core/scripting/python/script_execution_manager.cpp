#include "core/scripting/python/script_execution_manager.h"

#include "core/scripting/python/pyra_api_bridge.h"
#include "core/scripting/python/python_runner.h"
#include "core/scripting/python/python_runtime_manager.h"
#include "core/scripting/python/script_process_runner.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

namespace pyraqt::core {

ScriptExecutionManager::ScriptExecutionManager(PythonRuntimeManager &runtimeManager, PyraApiBridge &apiBridge, QObject *parent)
    : QObject(parent)
    , m_runtimeManager(runtimeManager)
    , m_apiBridge(apiBridge)
    , m_pythonRunner(new PythonRunner(runtimeManager, this))
    , m_runner(new ScriptProcessRunner(runtimeManager, apiBridge, this))
{
    connect(&m_runtimeManager, &PythonRuntimeManager::stdoutReceived, this, &ScriptExecutionManager::stdoutReceived);
    connect(&m_runtimeManager, &PythonRuntimeManager::stderrReceived, this, &ScriptExecutionManager::stderrReceived);
    connect(&m_runtimeManager, &PythonRuntimeManager::statusMessageRequested, this, &ScriptExecutionManager::statusMessageRequested);
    connect(&m_runtimeManager, &PythonRuntimeManager::dialogMessageRequested, this, &ScriptExecutionManager::dialogMessageRequested);
    connect(&m_runtimeManager, &PythonRuntimeManager::logMessageRequested, this, &ScriptExecutionManager::logMessageRequested);
    connect(m_runner, &ScriptProcessRunner::stdoutReady, this, &ScriptExecutionManager::stdoutReceived);
    connect(m_runner, &ScriptProcessRunner::stderrReady, this, &ScriptExecutionManager::stderrReceived);
    connect(m_runner, &ScriptProcessRunner::failed, this, &ScriptExecutionManager::executionFailed);
    connect(m_runner, &ScriptProcessRunner::finished, this, [this](int exitCode, QProcess::ExitStatus) {
        emit executionFinished(exitCode);
    });
    connect(m_runner, &ScriptProcessRunner::started, this, [this](const ScriptSession &session) {
        emit executionStarted(session.isSelection ? QStringLiteral("selection") : session.scriptPath);
    });
    connect(m_runner, &ScriptProcessRunner::commandFileUpdated, this, &ScriptExecutionManager::processCommandFile);
}

ScriptExecutionManager::~ScriptExecutionManager() = default;

bool ScriptExecutionManager::runFile(const QString &path, const QStringList &args)
{
    if (m_runtimeManager.useIsolatedExecutionByDefault()) {
        return runIsolatedFile(path, args);
    }

    emit executionStarted(path);
    const ScriptResult result = m_pythonRunner->runFile(path, args);
    if (!result.traceback.isEmpty()) {
        emit stderrReceived(result.traceback);
    }
    emit executionFinished(result.success ? 0 : 1);
    if (!result.success) {
        emit executionFailed(result.traceback.isEmpty() ? QStringLiteral("Python script execution failed") : result.traceback);
    }
    return result.success;
}

bool ScriptExecutionManager::runIsolatedFile(const QString &path, const QStringList &args)
{
    ScriptSession session;
    session.scriptPath = path;
    session.arguments = args;
    session.timeoutMs = m_runtimeManager.executionTimeoutMs();
    session.workingDirectory = QFileInfo(path).absolutePath();
    return m_runner->run(session);
}

ScriptResult ScriptExecutionManager::runCommand(const QString &command)
{
    emit executionStarted(QStringLiteral("command"));
    const ScriptResult result = m_pythonRunner->runCommand(command);
    if (!result.traceback.isEmpty()) {
        emit stderrReceived(result.traceback);
    }
    emit executionFinished(result.success ? 0 : 1);
    if (!result.success) {
        emit executionFailed(result.traceback.isEmpty() ? QStringLiteral("Python command execution failed") : result.traceback);
    }
    return result;
}

bool ScriptExecutionManager::runSelection(const QString &code, const QString &fileContext)
{
    Q_UNUSED(fileContext)
    emit executionStarted(QStringLiteral("selection"));
    const ScriptResult result = m_pythonRunner->runCode(code);
    if (!result.traceback.isEmpty()) {
        emit stderrReceived(result.traceback);
    }
    emit executionFinished(result.success ? 0 : 1);
    if (!result.success) {
        emit executionFailed(result.traceback.isEmpty() ? QStringLiteral("Python selection execution failed") : result.traceback);
    }
    return result.success;
}

bool ScriptExecutionManager::runIsolatedSelection(const QString &code, const QString &fileContext)
{
    ScriptSession session;
    session.isSelection = true;
    session.selectionCode = code;
    session.timeoutMs = m_runtimeManager.executionTimeoutMs();
    session.workingDirectory = fileContext.isEmpty() ? QString() : QFileInfo(fileContext).absolutePath();
    return m_runner->run(session);
}

void ScriptExecutionManager::stop()
{
    m_runner->stop();
}

bool ScriptExecutionManager::isRunning() const
{
    return m_runner->isRunning();
}

void ScriptExecutionManager::processCommandFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    static qint64 lastPosition = 0;
    file.seek(lastPosition);
    while (!file.atEnd()) {
        const QByteArray line = file.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        const QJsonDocument document = QJsonDocument::fromJson(line);
        if (!document.isObject()) {
            continue;
        }

        const QJsonObject root = document.object();
        const QString kind = root.value(QStringLiteral("kind")).toString();
        const QJsonObject payload = root.value(QStringLiteral("payload")).toObject();

        if (kind == QStringLiteral("set_status")) {
            emit statusMessageRequested(payload.value(QStringLiteral("text")).toString());
        } else if (kind == QStringLiteral("show_message")) {
            emit dialogMessageRequested(payload.value(QStringLiteral("text")).toString());
        } else if (kind == QStringLiteral("log")) {
            emit logMessageRequested(
                payload.value(QStringLiteral("level")).toString(),
                payload.value(QStringLiteral("text")).toString());
        }
    }
    lastPosition = file.pos();
}

} // namespace pyraqt::core

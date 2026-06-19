#include "core/scripting/script_execution_manager.h"

#include "core/scripting/pyra_api_bridge.h"
#include "core/scripting/python_runtime_manager.h"
#include "core/scripting/script_process_runner.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

namespace pyraqt::core {

ScriptExecutionManager::ScriptExecutionManager(PythonRuntimeManager &runtimeManager, PyraApiBridge &apiBridge, QObject *parent)
    : QObject(parent)
    , m_runtimeManager(runtimeManager)
    , m_apiBridge(apiBridge)
    , m_runner(new ScriptProcessRunner(runtimeManager, apiBridge, this))
{
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
    ScriptSession session;
    session.scriptPath = path;
    session.arguments = args;
    session.timeoutMs = m_runtimeManager.executionTimeoutMs();
    session.workingDirectory = QFileInfo(path).absolutePath();
    return m_runner->run(session);
}

bool ScriptExecutionManager::runSelection(const QString &code, const QString &fileContext)
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

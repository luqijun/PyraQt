#include "core/scripting/script_process_runner.h"

#include "core/scripting/pyra_api_bridge.h"
#include "core/scripting/python_runtime_manager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QTemporaryFile>
#include <QTimer>

namespace pyraqt::core {
namespace {

QString dataRootPath()
{
    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (environment.contains(QStringLiteral("PYRAQT_DATA_DIR"))) {
        return environment.value(QStringLiteral("PYRAQT_DATA_DIR"));
    }
    return QDir::temp().filePath(QStringLiteral("PyraQt"));
}

QString createTemporaryScript(const QString &content)
{
    QDir root(dataRootPath());
    root.mkpath(QStringLiteral("runtime"));
    QTemporaryFile file(root.filePath(QStringLiteral("runtime/selection-XXXXXX.py")));
    file.setAutoRemove(false);
    if (!file.open()) {
        return {};
    }
    file.write(content.toUtf8());
    file.close();
    return file.fileName();
}

QString createCommandFilePath()
{
    QDir root(dataRootPath());
    root.mkpath(QStringLiteral("runtime"));
    return root.filePath(QStringLiteral("runtime/pyra_commands.jsonl"));
}

} // namespace

ScriptProcessRunner::ScriptProcessRunner(PythonRuntimeManager &runtimeManager, PyraApiBridge &apiBridge, QObject *parent)
    : QObject(parent)
    , m_runtimeManager(runtimeManager)
    , m_apiBridge(apiBridge)
{
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
        emit stdoutReady(QString::fromUtf8(m_process.readAllStandardOutput()));
    });
    connect(&m_process, &QProcess::readyReadStandardError, this, [this] {
        emit stderrReady(QString::fromUtf8(m_process.readAllStandardError()));
    });
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](int exitCode, QProcess::ExitStatus exitStatus) {
            emit finished(exitCode, exitStatus);
            cleanupTransientFiles();
        });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        emit failed(m_process.errorString());
        cleanupTransientFiles();
    });
}

ScriptProcessRunner::~ScriptProcessRunner()
{
    stop();
}

bool ScriptProcessRunner::run(const ScriptSession &session)
{
    if (m_process.state() != QProcess::NotRunning) {
        emit failed(QStringLiteral("A script is already running"));
        return false;
    }

    if (!m_runtimeManager.isInterpreterAvailable()) {
        emit failed(QStringLiteral("Python interpreter is not available"));
        return false;
    }

    if (!m_apiBridge.ensureBridgeFiles()) {
        emit failed(QStringLiteral("Failed to prepare Pyra API bridge files"));
        return false;
    }

    const QString scriptPath = prepareScriptPath(session);
    if (scriptPath.isEmpty()) {
        emit failed(QStringLiteral("No script content available to run"));
        return false;
    }

    m_commandFilePath = createCommandFilePath();
    QFile::remove(m_commandFilePath);
    QFile commandFile(m_commandFilePath);
    commandFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    commandFile.close();

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("PYTHONPATH"),
        QStringLiteral("%1:%2").arg(m_apiBridge.bridgeRootPath(), environment.value(QStringLiteral("PYTHONPATH"))));
    environment.insert(QStringLiteral("PYRAQT_BRIDGE_COMMAND_FILE"), m_commandFilePath);
    environment.insert(QStringLiteral("PYRAQT_APP_NAME"), QStringLiteral("PyraQt"));
    environment.insert(QStringLiteral("PYRAQT_APP_VERSION"), QStringLiteral("0.2.0"));
    environment.insert(QStringLiteral("PYRAQT_PYTHON_PATH"), m_runtimeManager.interpreterPath());
    environment.insert(QStringLiteral("PYTHONUNBUFFERED"), QStringLiteral("1"));

    m_process.setProcessEnvironment(environment);
    m_process.setProgram(m_runtimeManager.interpreterPath());
    QStringList arguments{m_apiBridge.bootstrapFilePath(), scriptPath};
    arguments.append(session.arguments);
    m_process.setArguments(arguments);
    m_process.setWorkingDirectory(session.workingDirectory.isEmpty() ? QFileInfo(scriptPath).absolutePath() : session.workingDirectory);
    m_process.start();

    if (!m_process.waitForStarted(3000)) {
        emit failed(m_process.errorString());
        cleanupTransientFiles();
        return false;
    }

    emit commandFileUpdated(m_commandFilePath);
    emit started(session);

    if (session.timeoutMs > 0) {
        QTimer::singleShot(session.timeoutMs, this, [this] {
            if (m_process.state() != QProcess::NotRunning) {
                m_process.kill();
            }
        });
    }

    return true;
}

void ScriptProcessRunner::stop()
{
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(2000);
    }
    cleanupTransientFiles();
}

bool ScriptProcessRunner::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

QString ScriptProcessRunner::prepareScriptPath(const ScriptSession &session)
{
    if (session.isSelection) {
        m_selectionFilePath = createTemporaryScript(session.selectionCode);
        return m_selectionFilePath;
    }
    return session.scriptPath;
}

void ScriptProcessRunner::cleanupTransientFiles()
{
    if (!m_selectionFilePath.isEmpty()) {
        QFile::remove(m_selectionFilePath);
        m_selectionFilePath.clear();
    }
}

} // namespace pyraqt::core

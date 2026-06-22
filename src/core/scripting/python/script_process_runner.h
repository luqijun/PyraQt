#pragma once

#include "core/scripting/script_session.h"

#include <QObject>
#include <QProcess>

namespace pyraqt::core {

class PyraApiBridge;
class PythonRuntimeManager;

class ScriptProcessRunner final : public QObject {
    Q_OBJECT

public:
    ScriptProcessRunner(PythonRuntimeManager &runtimeManager, PyraApiBridge &apiBridge, QObject *parent = nullptr);
    ~ScriptProcessRunner() override;

    bool run(const ScriptSession &session);
    void stop();
    [[nodiscard]] bool isRunning() const;

signals:
    void started(const ScriptSession &session);
    void stdoutReady(const QString &output);
    void stderrReady(const QString &output);
    void commandFileUpdated(const QString &path);
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void failed(const QString &message);

private:
    QString prepareScriptPath(const ScriptSession &session);
    void cleanupTransientFiles();

    PythonRuntimeManager &m_runtimeManager;
    PyraApiBridge &m_apiBridge;
    QProcess m_process;
    QString m_commandFilePath;
    QString m_selectionFilePath;
};

} // namespace pyraqt::core

#pragma once

#include "core/scripting/script_session.h"

#include <QObject>

class QProcess;

namespace pyraqt::core {

class PyraApiBridge;
class PythonRuntimeManager;
class ScriptProcessRunner;

class ScriptExecutionManager final : public QObject {
    Q_OBJECT

public:
    ScriptExecutionManager(PythonRuntimeManager &runtimeManager, PyraApiBridge &apiBridge, QObject *parent = nullptr);
    ~ScriptExecutionManager() override;

    bool runFile(const QString &path, const QStringList &args = {});
    bool runSelection(const QString &code, const QString &fileContext = QString());
    void stop();
    [[nodiscard]] bool isRunning() const;

signals:
    void executionStarted(const QString &pathOrLabel);
    void stdoutReceived(const QString &output);
    void stderrReceived(const QString &output);
    void executionFinished(int exitCode);
    void executionFailed(const QString &message);
    void statusMessageRequested(const QString &message);
    void dialogMessageRequested(const QString &message);
    void logMessageRequested(const QString &level, const QString &message);

private:
    void processCommandFile(const QString &path);

    PythonRuntimeManager &m_runtimeManager;
    PyraApiBridge &m_apiBridge;
    ScriptProcessRunner *m_runner = nullptr;
};

} // namespace pyraqt::core

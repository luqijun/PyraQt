#pragma once

#include "core/scripting/script_result.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

struct _object;
using PyObject = _object;

namespace pyraqt::core {

class ConfigManager;

class PythonRuntimeManager final : public QObject {
    Q_OBJECT

public:
    explicit PythonRuntimeManager(ConfigManager &configManager, QObject *parent = nullptr);
    ~PythonRuntimeManager() override;

    [[nodiscard]] QString interpreterPath() const;
    void setInterpreterPath(const QString &path);
    void setExecutionTimeoutMs(int timeoutMs);

    [[nodiscard]] bool isInterpreterAvailable() const;
    [[nodiscard]] QString pythonVersion() const;
    [[nodiscard]] int executionTimeoutMs() const;
    [[nodiscard]] bool isEnabled() const;
    [[nodiscard]] bool isEmbeddedInitialized() const;
    [[nodiscard]] bool macrosEnabled() const;
    void setMacrosEnabled(bool enabled);
    [[nodiscard]] bool fileSystemAccessEnabled() const;
    void setFileSystemAccessEnabled(bool enabled);
    [[nodiscard]] bool useIsolatedExecutionByDefault() const;
    void setUseIsolatedExecutionByDefault(bool enabled);
    [[nodiscard]] int consoleHistoryLimit() const;
    void setConsoleHistoryLimit(int limit);
    [[nodiscard]] bool codeCompletionEnabled() const;
    void setCodeCompletionEnabled(bool enabled);
    [[nodiscard]] int completionTriggerThreshold() const;
    void setCompletionTriggerThreshold(int threshold);
    [[nodiscard]] QStringList pluginPaths() const;
    [[nodiscard]] QString pythonPath() const;
    [[nodiscard]] PyObject *globalDict() const;

    bool initializeEmbedded(const QStringList &extraPythonPaths = {}, const QStringList &extraPluginPaths = {});
    void shutdownEmbedded();
    [[nodiscard]] ScriptResult runString(const QString &code, bool singleInput = false);
    [[nodiscard]] ScriptResult runFile(const QString &filePath, const QStringList &arguments = {});
    [[nodiscard]] ScriptResult evalString(const QString &expression);
    [[nodiscard]] QStringList completeMembers(const QString &objectExpression, const QString &contextCode = {});
    [[nodiscard]] bool setArgv(const QStringList &arguments);
    [[nodiscard]] QString getTraceback();
    [[nodiscard]] QString getErrorText();
    void injectGlobalObject(const QString &name, PyObject *object);
    void setRegistrationOwnerId(const QString &ownerId);
    [[nodiscard]] QString registrationOwnerId() const;

signals:
    void interpreterChanged(const QString &path);
    void executionTimeoutChanged(int timeoutMs);
    void embeddedStateChanged(bool enabled);
    void stdoutReceived(const QString &output);
    void stderrReceived(const QString &output);
    void statusMessageRequested(const QString &message);
    void dialogMessageRequested(const QString &message);
    void logMessageRequested(const QString &level, const QString &message);
    void commandRegistrationRequested(const QString &ownerId, const QString &id, const QString &title, const QString &description, const QStringList &keywords);
    void filesystemAccessViolation(const QString &operation, const QString &path);
    void macroLoadRequested(const QString &code);
    void macroTriggerRequested(const QString &name);
    void expressionRegistrationRequested(const QString &ownerId, const QString &name, const QString &code);
    void expressionEvaluationRequested(const QString &name, const QStringList &arguments);
    void processingRegistrationRequested(const QString &ownerId, const QString &id, const QString &code);
    void processingRunRequested(const QString &id, const QVariantMap &parameters);

private:
    bool appendPyraNativeModule();
    bool installPyraPackage(const QStringList &extraPythonPaths, const QStringList &extraPluginPaths);
    bool configureSysPath(const QStringList &extraPythonPaths, const QStringList &extraPluginPaths);
    [[nodiscard]] QString pyObjectToString(PyObject *object) const;
    [[nodiscard]] QStringList defaultPythonPaths() const;
    [[nodiscard]] QStringList defaultPluginPaths() const;
    void emitStdoutFromPython(const QString &output);
    void emitStderrFromPython(const QString &output);
    void emitStatusFromPython(const QString &message);
    void emitDialogFromPython(const QString &message);
    void emitLogFromPython(const QString &level, const QString &message);
    void emitCommandRegistrationFromPython(const QString &id, const QString &title, const QString &description, const QStringList &keywords);
    bool isFileSystemAccessAllowedFromPython(const QString &operation, const QString &path);
    void emitMacroLoadFromPython(const QString &code);
    void emitMacroTriggerFromPython(const QString &name);
    void emitExpressionRegistrationFromPython(const QString &name, const QString &code);
    void emitExpressionEvaluationFromPython(const QString &name, const QStringList &arguments);
    void emitProcessingRegistrationFromPython(const QString &id, const QString &code);
    void emitProcessingRunFromPython(const QString &id, const QVariantMap &parameters);

    ConfigManager &m_configManager;
    PyObject *m_mainModule = nullptr;
    PyObject *m_globalDict = nullptr;
    void *m_mainThreadState = nullptr;
    bool m_embeddedInitialized = false;
    QString m_registrationOwnerId;

    friend PyObject *pyraqtNativeEmitStdout(PyObject *, PyObject *);
    friend PyObject *pyraqtNativeEmitStderr(PyObject *, PyObject *);
    friend PyObject *pyraqtNativeSetStatus(PyObject *, PyObject *);
    friend PyObject *pyraqtNativeShowMessage(PyObject *, PyObject *);
    friend PyObject *pyraqtNativeLog(PyObject *, PyObject *);
    friend PyObject *pyraqtNativeRegisterCommand(PyObject *, PyObject *);
    friend PyObject *pyraqtNativeCanAccessFileSystem(PyObject *, PyObject *);
    friend PyObject *pyraqtNativeLoadMacro(PyObject *, PyObject *);
    friend PyObject *pyraqtNativeTriggerMacro(PyObject *, PyObject *);
    friend PyObject *pyraqtNativeRegisterExpression(PyObject *, PyObject *);
    friend PyObject *pyraqtNativeEvaluateExpression(PyObject *, PyObject *);
    friend PyObject *pyraqtNativeRegisterProcessing(PyObject *, PyObject *);
    friend PyObject *pyraqtNativeRunProcessing(PyObject *, PyObject *);
};

} // namespace pyraqt::core

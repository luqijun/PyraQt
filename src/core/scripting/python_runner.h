#pragma once

#include "core/scripting/script_result.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace pyraqt::core {

class PythonRuntimeManager;

class PythonRunner final : public QObject {
    Q_OBJECT

public:
    explicit PythonRunner(PythonRuntimeManager &runtimeManager, QObject *parent = nullptr);

    [[nodiscard]] ScriptResult runCode(const QString &code);
    [[nodiscard]] ScriptResult runCommand(const QString &command);
    [[nodiscard]] ScriptResult eval(const QString &expression);
    [[nodiscard]] ScriptResult runFile(const QString &path, const QStringList &arguments = {});
    [[nodiscard]] ScriptResult callFunction(const QString &moduleName, const QString &functionName, const QStringList &arguments = {});
    bool setGlobal(const QString &name, const QString &value);

private:
    PythonRuntimeManager &m_runtimeManager;
};

} // namespace pyraqt::core

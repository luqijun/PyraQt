#include "core/scripting/python_runner.h"

#include "core/scripting/python_runtime_manager.h"

namespace pyraqt::core {

PythonRunner::PythonRunner(PythonRuntimeManager &runtimeManager, QObject *parent)
    : QObject(parent)
    , m_runtimeManager(runtimeManager)
{
}

ScriptResult PythonRunner::runCode(const QString &code)
{
    return m_runtimeManager.runString(code, false);
}

ScriptResult PythonRunner::runCommand(const QString &command)
{
    ScriptResult result = m_runtimeManager.evalString(command);
    if (result.success) {
        return result;
    }
    return m_runtimeManager.runString(command, true);
}

ScriptResult PythonRunner::eval(const QString &expression)
{
    return m_runtimeManager.evalString(expression);
}

ScriptResult PythonRunner::runFile(const QString &path, const QStringList &arguments)
{
    return m_runtimeManager.runFile(path, arguments);
}

ScriptResult PythonRunner::callFunction(const QString &moduleName, const QString &functionName, const QStringList &arguments)
{
    QStringList quotedArguments;
    quotedArguments.reserve(arguments.size());
    for (QString argument : arguments) {
        argument.replace('\\', QStringLiteral("\\\\"));
        argument.replace('\'', QStringLiteral("\\'"));
        quotedArguments.push_back(QStringLiteral("'%1'").arg(argument));
    }

    const QString code = QStringLiteral(
                             "import importlib\n"
                             "_pyraqt_call_module = importlib.import_module('%1')\n"
                             "_pyraqt_call_result = getattr(_pyraqt_call_module, '%2')(%3)\n"
                             "_pyraqt_call_result\n")
                             .arg(moduleName, functionName, quotedArguments.join(QStringLiteral(", ")));
    return m_runtimeManager.runString(code, false);
}

bool PythonRunner::setGlobal(const QString &name, const QString &value)
{
    QString escaped = value;
    escaped.replace('\\', QStringLiteral("\\\\"));
    escaped.replace('\'', QStringLiteral("\\'"));
    return m_runtimeManager.runString(QStringLiteral("%1 = '%2'").arg(name, escaped), false).success;
}

} // namespace pyraqt::core

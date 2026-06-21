#include "core/scripting/python_feature_manager.h"

#include "core/scripting/python_runner.h"
#include "core/scripting/python_runtime_manager.h"

#include <QMetaType>
#include <QTimer>

namespace pyraqt::core {

PythonFeatureManager::PythonFeatureManager(PythonRuntimeManager &runtimeManager, PythonRunner &runner, QObject *parent)
    : QObject(parent)
    , m_runtimeManager(runtimeManager)
    , m_runner(runner)
{
    qRegisterMetaType<pyraqt::core::ScriptResult>("pyraqt::core::ScriptResult");
    qRegisterMetaType<pyraqt::core::ScriptResult>("ScriptResult");
}

bool PythonFeatureManager::loadMacros(const QString &code)
{
    if (!m_runtimeManager.macrosEnabled()) {
        return false;
    }
    const QString wrapped = QStringLiteral(
                                "import types, sys\n"
                                "proj_macros_mod = types.ModuleType('proj_macros_mod')\n"
                                "exec(%1, proj_macros_mod.__dict__)\n"
                                "sys.modules['proj_macros_mod'] = proj_macros_mod\n")
                                .arg(quote(code));
    return m_runner.runCode(wrapped).success;
}

ScriptResult PythonFeatureManager::triggerMacro(const QString &name)
{
    if (!m_runtimeManager.macrosEnabled()) {
        ScriptResult result;
        result.traceback = QStringLiteral("Python macros are disabled.");
        result.exitCode = 1;
        return result;
    }
    const QString code = QStringLiteral(
                             "import sys\n"
                             "mod = sys.modules.get('proj_macros_mod')\n"
                             "getattr(mod, '%1')() if mod is not None and hasattr(mod, '%1') else None\n")
                             .arg(name);
    return m_runner.runCode(code);
}

bool PythonFeatureManager::unloadMacros()
{
    return m_runner.runCode(QStringLiteral("import sys\nsys.modules.pop('proj_macros_mod', None)\n")).success;
}

bool PythonFeatureManager::registerExpressionFunction(const QString &name, const QString &code, const QString &ownerId)
{
    const bool ok = m_runner.runCode(code).success;
    if (ok) {
        m_expressionFunctions.insert(name, code);
        m_expressionOwners.insert(name, ownerId);
    }
    return ok;
}

ScriptResult PythonFeatureManager::evaluateExpressionFunction(const QString &name, const QStringList &arguments)
{
    QStringList quotedArguments;
    for (const QString &argument : arguments) {
        quotedArguments.push_back(quote(argument));
    }
    return m_runner.eval(QStringLiteral("%1(%2)").arg(name, quotedArguments.join(QStringLiteral(", "))));
}

void PythonFeatureManager::unregisterExpressionFunctions(const QString &ownerId)
{
    for (auto it = m_expressionOwners.begin(); it != m_expressionOwners.end();) {
        if (it.value() != ownerId) {
            ++it;
            continue;
        }
        m_expressionFunctions.remove(it.key());
        it = m_expressionOwners.erase(it);
    }
}

bool PythonFeatureManager::registerProcessingAlgorithm(const QString &id, const QString &code, const QString &ownerId)
{
    const bool ok = m_runner.runCode(code).success;
    if (ok) {
        m_processingAlgorithms.insert(id, code);
        m_processingOwners.insert(id, ownerId);
    }
    return ok;
}

ScriptResult PythonFeatureManager::runProcessingAlgorithm(const QString &id, const QVariantMap &parameters)
{
    QStringList items;
    for (auto it = parameters.constBegin(); it != parameters.constEnd(); ++it) {
        items.push_back(QStringLiteral("%1: %2").arg(quote(it.key()), quote(it.value().toString())));
    }
    const QString parametersLiteral = QStringLiteral("{%1}").arg(items.join(QStringLiteral(", ")));
    return m_runner.eval(QStringLiteral("%1(%2)").arg(id, parametersLiteral));
}

void PythonFeatureManager::runProcessingAlgorithmAsync(const QString &id, const QVariantMap &parameters)
{
    emit processingStarted(id);
    QTimer::singleShot(0, this, [this, id, parameters] {
        const ScriptResult result = runProcessingAlgorithm(id, parameters);
        emit processingFinished(id, result);
    });
}

void PythonFeatureManager::unregisterProcessingAlgorithms(const QString &ownerId)
{
    for (auto it = m_processingOwners.begin(); it != m_processingOwners.end();) {
        if (it.value() != ownerId) {
            ++it;
            continue;
        }
        m_processingAlgorithms.remove(it.key());
        it = m_processingOwners.erase(it);
    }
}

QString PythonFeatureManager::quote(const QString &value) const
{
    QString escaped = value;
    escaped.replace('\\', QStringLiteral("\\\\"));
    escaped.replace('\'', QStringLiteral("\\'"));
    escaped.replace('\n', QStringLiteral("\\n"));
    return QStringLiteral("'%1'").arg(escaped);
}

} // namespace pyraqt::core

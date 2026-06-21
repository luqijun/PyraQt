#pragma once

#include "core/scripting/script_result.h"

#include <QObject>
#include <QMap>
#include <QString>
#include <QVariantMap>

namespace pyraqt::core {

class PythonRunner;
class PythonRuntimeManager;

class PythonFeatureManager final : public QObject {
    Q_OBJECT

public:
    PythonFeatureManager(PythonRuntimeManager &runtimeManager, PythonRunner &runner, QObject *parent = nullptr);

    bool loadMacros(const QString &code);
    ScriptResult triggerMacro(const QString &name);
    bool unloadMacros();

    bool registerExpressionFunction(const QString &name, const QString &code);
    ScriptResult evaluateExpressionFunction(const QString &name, const QStringList &arguments = {});

    bool registerProcessingAlgorithm(const QString &id, const QString &code);
    ScriptResult runProcessingAlgorithm(const QString &id, const QVariantMap &parameters = {});
    void runProcessingAlgorithmAsync(const QString &id, const QVariantMap &parameters = {});

signals:
    void processingStarted(const QString &id);
    void processingFinished(const QString &id, const ScriptResult &result);
    void processingProgressChanged(const QString &id, int progress);

private:
    QString quote(const QString &value) const;

    PythonRuntimeManager &m_runtimeManager;
    PythonRunner &m_runner;
    QMap<QString, QString> m_expressionFunctions;
    QMap<QString, QString> m_processingAlgorithms;
};

} // namespace pyraqt::core

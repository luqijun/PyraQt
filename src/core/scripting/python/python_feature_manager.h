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

    bool registerExpressionFunction(const QString &name, const QString &code, const QString &ownerId = QStringLiteral("python"));
    ScriptResult evaluateExpressionFunction(const QString &name, const QStringList &arguments = {});
    void unregisterExpressionFunctions(const QString &ownerId);

    bool registerProcessingAlgorithm(const QString &id, const QString &code, const QString &ownerId = QStringLiteral("python"));
    ScriptResult runProcessingAlgorithm(const QString &id, const QVariantMap &parameters = {});
    void runProcessingAlgorithmAsync(const QString &id, const QVariantMap &parameters = {});
    void unregisterProcessingAlgorithms(const QString &ownerId);

signals:
    void processingStarted(const QString &id);
    void processingFinished(const QString &id, const ScriptResult &result);
    void processingProgressChanged(const QString &id, int progress);

private:
    QString quote(const QString &value) const;

    PythonRuntimeManager &m_runtimeManager;
    PythonRunner &m_runner;
    QMap<QString, QString> m_expressionFunctions;
    QMap<QString, QString> m_expressionOwners;
    QMap<QString, QString> m_processingAlgorithms;
    QMap<QString, QString> m_processingOwners;
};

} // namespace pyraqt::core

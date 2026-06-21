#pragma once

#include <QDialog>

class QLineEdit;
class QPlainTextEdit;
class QTextEdit;

namespace pyraqt::core {
class PythonRuntimeManager;
class ScriptExecutionManager;
} // namespace pyraqt::core

namespace pyraqt::ui {

class PythonToolsDialog final : public QDialog {
    Q_OBJECT

public:
    PythonToolsDialog(core::PythonRuntimeManager &runtimeManager, core::ScriptExecutionManager &executionManager, QWidget *parent = nullptr);

private:
    QWidget *createMacrosPage();
    QWidget *createExpressionsPage();
    QWidget *createProcessingPage();
    void appendOutput(const QString &message);

    core::PythonRuntimeManager &m_runtimeManager;
    core::ScriptExecutionManager &m_executionManager;
    QTextEdit *m_macroCodeEdit = nullptr;
    QLineEdit *m_macroNameEdit = nullptr;
    QLineEdit *m_expressionNameEdit = nullptr;
    QLineEdit *m_expressionArgsEdit = nullptr;
    QTextEdit *m_expressionCodeEdit = nullptr;
    QLineEdit *m_processingIdEdit = nullptr;
    QLineEdit *m_processingParamsEdit = nullptr;
    QTextEdit *m_processingCodeEdit = nullptr;
    QPlainTextEdit *m_output = nullptr;
};

} // namespace pyraqt::ui

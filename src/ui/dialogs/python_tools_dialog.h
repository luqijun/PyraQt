#pragma once

#include <QDialog>

class QDialogButtonBox;
class QEvent;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTabWidget;
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

protected:
    void changeEvent(QEvent *event) override;

private:
    void retranslateUi();
    QWidget *createMacrosPage();
    QWidget *createExpressionsPage();
    QWidget *createProcessingPage();
    void appendOutput(const QString &message);

    core::PythonRuntimeManager &m_runtimeManager;
    core::ScriptExecutionManager &m_executionManager;
    QTabWidget *m_tabs = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
    QLabel *m_macrosInfoLabel = nullptr;
    QTextEdit *m_macroCodeEdit = nullptr;
    QLineEdit *m_macroNameEdit = nullptr;
    QPushButton *m_loadMacroButton = nullptr;
    QPushButton *m_triggerMacroButton = nullptr;
    QFormLayout *m_expressionForm = nullptr;
    QLineEdit *m_expressionNameEdit = nullptr;
    QLineEdit *m_expressionArgsEdit = nullptr;
    QTextEdit *m_expressionCodeEdit = nullptr;
    QPushButton *m_registerExpressionButton = nullptr;
    QPushButton *m_evaluateExpressionButton = nullptr;
    QFormLayout *m_processingForm = nullptr;
    QLineEdit *m_processingIdEdit = nullptr;
    QLineEdit *m_processingParamsEdit = nullptr;
    QTextEdit *m_processingCodeEdit = nullptr;
    QPushButton *m_registerProcessingButton = nullptr;
    QPushButton *m_runProcessingButton = nullptr;
    QPlainTextEdit *m_output = nullptr;
};

} // namespace pyraqt::ui

#pragma once

#include <QWidget>

class QPlainTextEdit;
class QPushButton;
class QTextEdit;

namespace pyraqt::ui {
class PythonCompletionLineEdit;
class PythonCompletionTextEdit;
}

namespace pyraqt::core {
class PythonRuntimeManager;
class ScriptExecutionManager;
} // namespace pyraqt::core

namespace pyraqt::ui {

class PythonConsoleWidget final : public QWidget {
    Q_OBJECT

public:
    PythonConsoleWidget(core::PythonRuntimeManager &runtimeManager, core::ScriptExecutionManager &executionManager, QWidget *parent = nullptr);

    void appendOutput(const QString &prefix, const QString &message);
    [[nodiscard]] bool inputCompletionEnabled() const;
    [[nodiscard]] bool editorCompletionEnabled() const;
    [[nodiscard]] QStringList completionWords() const;
    [[nodiscard]] QString inputCompletionPrefixForTesting() const;
    [[nodiscard]] QStringList inputCompletionCandidatesForTesting() const;
    [[nodiscard]] QStringList memberCompletionsForTesting(const QString &prefix, const QString &contextCode = {}) const;
    [[nodiscard]] QString outputTextForTesting() const;
    void submitCommandForTesting(const QString &command);

private:
    void refreshCompletions();
    void runCommand();
    void runEditor();
    void inspectObjects();
    void clearConsole();
    [[nodiscard]] QStringList memberCompletionsForPrefix(const QString &prefix, const QString &contextCode = {}) const;
    bool eventFilter(QObject *watched, QEvent *event) override;

    core::PythonRuntimeManager &m_runtimeManager;
    core::ScriptExecutionManager &m_executionManager;
    QPlainTextEdit *m_output = nullptr;
    PythonCompletionTextEdit *m_editor = nullptr;
    PythonCompletionLineEdit *m_input = nullptr;
    QStringList m_completionWords;
    QStringList m_history;
    int m_historyIndex = 0;
};

} // namespace pyraqt::ui

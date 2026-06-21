#pragma once

#include <QWidget>

class QPlainTextEdit;
class QTextEdit;
class QToolBar;
class QToolButton;

namespace pyraqt::ui {
class PythonCompletionLineEdit;
class PythonCompletionTextEdit;
}

namespace pyraqt::core {
class PythonRuntimeManager;
class ScriptExecutionManager;
class ThemeManager;
} // namespace pyraqt::core

namespace pyraqt::ui {

class PythonConsoleWidget final : public QWidget {
    Q_OBJECT

public:
    PythonConsoleWidget(
        core::PythonRuntimeManager &runtimeManager,
        core::ScriptExecutionManager &executionManager,
        core::ThemeManager *themeManager = nullptr,
        QWidget *parent = nullptr);

    void appendOutput(const QString &prefix, const QString &message);
    [[nodiscard]] bool inputCompletionEnabled() const;
    [[nodiscard]] bool editorCompletionEnabled() const;
    [[nodiscard]] QStringList completionWords() const;
    [[nodiscard]] QString inputCompletionPrefixForTesting() const;
    [[nodiscard]] QStringList inputCompletionCandidatesForTesting() const;
    [[nodiscard]] QStringList memberCompletionsForTesting(const QString &prefix, const QString &contextCode = {}) const;
    [[nodiscard]] QString outputTextForTesting() const;
    void submitCommandForTesting(const QString &command);
    [[nodiscard]] bool actionButtonsShowText() const;
    [[nodiscard]] bool actionButtonsHaveIcons() const;
    void clearOutput();

protected:
    void changeEvent(QEvent *event) override;

private:
    void refreshCompletions();
    void retranslateUi();
    void applyIcons();
    void runCommand();
    void runEditor();
    void inspectObjects();
    void clearConsole();
    [[nodiscard]] QStringList memberCompletionsForPrefix(const QString &prefix, const QString &contextCode = {}) const;
    bool eventFilter(QObject *watched, QEvent *event) override;

    core::PythonRuntimeManager &m_runtimeManager;
    core::ScriptExecutionManager &m_executionManager;
    core::ThemeManager *m_themeManager = nullptr;
    QPlainTextEdit *m_output = nullptr;
    PythonCompletionTextEdit *m_editor = nullptr;
    PythonCompletionLineEdit *m_input = nullptr;
    QToolBar *m_actionToolBar = nullptr;
    QToolButton *m_runCommandButton = nullptr;
    QToolButton *m_runEditorButton = nullptr;
    QToolButton *m_inspectButton = nullptr;
    QToolButton *m_clearButton = nullptr;
    QStringList m_completionWords;
    QStringList m_history;
    int m_historyIndex = 0;
    bool m_outputUsesDefaultMessage = true;
};

} // namespace pyraqt::ui

#include "ui/panels/python/python_console_widget.h"

#include "core/scripting/python_completion_provider.h"
#include "core/scripting/python_runtime_manager.h"
#include "core/scripting/script_result.h"
#include "core/scripting/script_execution_manager.h"
#include "ui/common/python_completion_line_edit.h"
#include "ui/common/python_completion_text_edit.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>

namespace pyraqt::ui {

PythonConsoleWidget::PythonConsoleWidget(
    core::PythonRuntimeManager &runtimeManager,
    core::ScriptExecutionManager &executionManager,
    QWidget *parent)
    : QWidget(parent)
    , m_runtimeManager(runtimeManager)
    , m_executionManager(executionManager)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *splitter = new QSplitter(Qt::Vertical, this);
    m_output = new QPlainTextEdit(splitter);
    m_output->setReadOnly(true);
    m_output->setPlainText(tr("Python Console ready. Use pyra or iface to access the embedded PyraQt API."));
    m_editor = new PythonCompletionTextEdit(splitter);
    m_editor->setPlaceholderText(tr("Optional multi-line Python editor. Click Run Editor to execute in the shared interpreter."));
    m_editor->setMemberCompletionProvider([this](const QString &prefix, const QString &contextCode) {
        return memberCompletionsForPrefix(prefix, contextCode);
    });
    splitter->addWidget(m_output);
    splitter->addWidget(m_editor);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter);

    auto *inputLayout = new QHBoxLayout();
    m_input = new PythonCompletionLineEdit(this);
    m_input->setPlaceholderText(tr(">>> Enter Python command"));
    m_input->installEventFilter(this);
    m_input->setMemberCompletionProvider([this](const QString &prefix) {
        return memberCompletionsForPrefix(prefix);
    });
    auto *runButton = new QPushButton(tr("Run Command"), this);
    auto *runEditorButton = new QPushButton(tr("Run Editor"), this);
    auto *inspectButton = new QPushButton(tr("Inspect"), this);
    auto *clearButton = new QPushButton(tr("Clear"), this);
    inputLayout->addWidget(m_input, 1);
    inputLayout->addWidget(runButton);
    inputLayout->addWidget(runEditorButton);
    inputLayout->addWidget(inspectButton);
    inputLayout->addWidget(clearButton);
    layout->addLayout(inputLayout);

    connect(m_input, &QLineEdit::returnPressed, this, &PythonConsoleWidget::runCommand);
    connect(runButton, &QPushButton::clicked, this, &PythonConsoleWidget::runCommand);
    connect(runEditorButton, &QPushButton::clicked, this, &PythonConsoleWidget::runEditor);
    connect(inspectButton, &QPushButton::clicked, this, &PythonConsoleWidget::inspectObjects);
    connect(clearButton, &QPushButton::clicked, this, &PythonConsoleWidget::clearConsole);
    connect(&m_runtimeManager, &core::PythonRuntimeManager::stdoutReceived, this, [this](const QString &output) {
        appendOutput(QStringLiteral("stdout"), output.trimmed());
    });
    connect(&m_runtimeManager, &core::PythonRuntimeManager::stderrReceived, this, [this](const QString &output) {
        appendOutput(QStringLiteral("stderr"), output.trimmed());
    });
    m_runtimeManager.initializeEmbedded();
    refreshCompletions();
}

void PythonConsoleWidget::appendOutput(const QString &prefix, const QString &message)
{
    if (message.isEmpty()) {
        return;
    }
    m_output->appendPlainText(QStringLiteral("[%1] %2").arg(prefix, message));
}

bool PythonConsoleWidget::inputCompletionEnabled() const
{
    return m_input != nullptr && m_input->hasCompleter();
}

bool PythonConsoleWidget::editorCompletionEnabled() const
{
    return m_editor != nullptr && m_editor->hasCompleter();
}

QStringList PythonConsoleWidget::completionWords() const
{
    return m_completionWords;
}

QString PythonConsoleWidget::inputCompletionPrefixForTesting() const
{
    return m_input == nullptr ? QString() : m_input->completionPrefixForTesting();
}

QStringList PythonConsoleWidget::inputCompletionCandidatesForTesting() const
{
    return m_input == nullptr ? QStringList{} : m_input->completionCandidatesForTesting();
}

QStringList PythonConsoleWidget::memberCompletionsForTesting(const QString &prefix, const QString &contextCode) const
{
    return memberCompletionsForPrefix(prefix, contextCode);
}

QString PythonConsoleWidget::outputTextForTesting() const
{
    return m_output == nullptr ? QString() : m_output->toPlainText();
}

void PythonConsoleWidget::submitCommandForTesting(const QString &command)
{
    if (m_input == nullptr) {
        return;
    }
    m_input->setText(command);
    runCommand();
}

void PythonConsoleWidget::refreshCompletions()
{
    if (!m_runtimeManager.codeCompletionEnabled()) {
        m_completionWords.clear();
        if (m_input != nullptr) {
            m_input->setCompletionWords({});
        }
        if (m_editor != nullptr) {
            m_editor->setCompletionWords({});
        }
        return;
    }

    pyraqt::core::PythonCompletionProvider provider;
    m_completionWords = provider.allCompletions(&m_runtimeManager);
    if (m_input != nullptr) {
        m_input->setCompletionWords(m_completionWords);
        m_input->setCompletionThreshold(m_runtimeManager.completionTriggerThreshold());
    }
    if (m_editor != nullptr) {
        m_editor->setCompletionWords(m_completionWords);
        m_editor->setCompletionThreshold(m_runtimeManager.completionTriggerThreshold());
    }
}

void PythonConsoleWidget::runCommand()
{
    const QString command = m_input->text().trimmed();
    if (command.isEmpty()) {
        return;
    }
    appendOutput(QStringLiteral("input"), QStringLiteral(">>> %1").arg(command));
    m_history.push_back(command);
    const int limit = m_runtimeManager.consoleHistoryLimit();
    while (m_history.size() > limit) {
        m_history.removeFirst();
    }
    m_historyIndex = m_history.size();
    m_input->clear();
    const core::ScriptResult result = m_executionManager.runCommand(command);
    if (result.success && !result.resultText.isEmpty() && result.resultText != QStringLiteral("None")) {
        appendOutput(QStringLiteral("result"), result.resultText);
    }
    refreshCompletions();
}

void PythonConsoleWidget::runEditor()
{
    const QString code = m_editor->toPlainText();
    if (code.trimmed().isEmpty()) {
        return;
    }
    appendOutput(QStringLiteral("input"), tr("Running editor buffer"));
    m_executionManager.runSelection(code);
    refreshCompletions();
}

void PythonConsoleWidget::inspectObjects()
{
    appendOutput(QStringLiteral("input"), tr("Inspecting Python globals"));
    m_executionManager.runSelection(QStringLiteral("print('\\n'.join(name for name in sorted(globals()) if not name.startswith('__')))"));
}

void PythonConsoleWidget::clearConsole()
{
    m_output->clear();
}

QStringList PythonConsoleWidget::memberCompletionsForPrefix(const QString &prefix, const QString &contextCode) const
{
    pyraqt::core::PythonCompletionProvider provider;
    const QString objectExpression = core::PythonCompletionProvider::objectExpressionForDottedPrefix(prefix);
    if (objectExpression.isEmpty()) {
        return {};
    }

    const QString memberPrefix = core::PythonCompletionProvider::memberNamePrefixForDottedPrefix(prefix);
    QStringList members = m_runtimeManager.completeMembers(objectExpression, contextCode);
    if (members.isEmpty()) {
        members = provider.commonMemberNames();
    }
    if (!memberPrefix.isEmpty()) {
        QStringList filtered;
        for (const QString &member : members) {
            if (member.startsWith(memberPrefix, Qt::CaseInsensitive)) {
                filtered.push_back(member);
            }
        }
        members = filtered;
    }

    QStringList words = core::PythonCompletionProvider::prefixedMemberCompletions(objectExpression, members);
    if (words.isEmpty() && memberPrefix.isEmpty()) {
        words = core::PythonCompletionProvider::prefixedMemberCompletions(objectExpression, provider.commonMemberNames());
    }
    return words;
}

bool PythonConsoleWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_input && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Up && !m_history.isEmpty()) {
            m_historyIndex = qMax(0, m_historyIndex - 1);
            m_input->setText(m_history.at(m_historyIndex));
            return true;
        }
        if (keyEvent->key() == Qt::Key_Down && !m_history.isEmpty()) {
            m_historyIndex = qMin(m_history.size(), m_historyIndex + 1);
            m_input->setText(m_historyIndex == m_history.size() ? QString() : m_history.at(m_historyIndex));
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace pyraqt::ui

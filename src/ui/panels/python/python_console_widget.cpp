#include "ui/panels/python/python_console_widget.h"

#include "core/scripting/python/python_completion_provider.h"
#include "core/scripting/python/python_runtime_manager.h"
#include "core/scripting/script_result.h"
#include "core/scripting/python/script_execution_manager.h"
#include "core/theme/theme_manager.h"
#include "ui/common/icon_utils.h"
#include "ui/common/python_completion_line_edit.h"
#include "ui/common/python_completion_text_edit.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace pyraqt::ui {

PythonConsoleWidget::PythonConsoleWidget(
    core::PythonRuntimeManager &runtimeManager,
    core::ScriptExecutionManager &executionManager,
    core::ThemeManager *themeManager,
    QWidget *parent)
    : QWidget(parent)
    , m_runtimeManager(runtimeManager)
    , m_executionManager(executionManager)
    , m_themeManager(themeManager)
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
    m_input->installEventFilter(this);
    m_input->setMemberCompletionProvider([this](const QString &prefix) {
        return memberCompletionsForPrefix(prefix);
    });
    inputLayout->addWidget(m_input, 1);
    m_actionToolBar = new QToolBar(this);
    m_actionToolBar->setObjectName(QStringLiteral("pythonConsoleToolBar"));
    m_actionToolBar->setIconSize(QSize(20, 20));
    m_actionToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_actionToolBar->setMovable(false);
    m_actionToolBar->setFloatable(false);
    m_actionToolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    m_runCommandButton = new QToolButton(m_actionToolBar);
    m_runEditorButton = new QToolButton(m_actionToolBar);
    m_inspectButton = new QToolButton(m_actionToolBar);
    m_clearButton = new QToolButton(m_actionToolBar);
    for (QToolButton *button : {m_runCommandButton, m_runEditorButton, m_inspectButton, m_clearButton}) {
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        m_actionToolBar->addWidget(button);
    }
    inputLayout->addWidget(m_actionToolBar);
    layout->addLayout(inputLayout);

    connect(m_input, &QLineEdit::returnPressed, this, &PythonConsoleWidget::runCommand);
    connect(m_runCommandButton, &QToolButton::clicked, this, &PythonConsoleWidget::runCommand);
    connect(m_runEditorButton, &QToolButton::clicked, this, &PythonConsoleWidget::runEditor);
    connect(m_inspectButton, &QToolButton::clicked, this, &PythonConsoleWidget::inspectObjects);
    connect(m_clearButton, &QToolButton::clicked, this, &PythonConsoleWidget::clearConsole);
    connect(&m_runtimeManager, &core::PythonRuntimeManager::stdoutReceived, this, [this](const QString &output) {
        appendOutput(QStringLiteral("stdout"), output.trimmed());
    });
    connect(&m_runtimeManager, &core::PythonRuntimeManager::stderrReceived, this, [this](const QString &output) {
        appendOutput(QStringLiteral("stderr"), output.trimmed());
    });
    if (m_themeManager != nullptr) {
        connect(m_themeManager, &core::ThemeManager::themeChanged, this, [this] {
            applyIcons();
        });
    }
    retranslateUi();
    applyIcons();
    m_runtimeManager.initializeEmbedded();
    refreshCompletions();
}

void PythonConsoleWidget::appendOutput(const QString &prefix, const QString &message)
{
    if (message.isEmpty()) {
        return;
    }
    m_outputUsesDefaultMessage = false;
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

bool PythonConsoleWidget::actionButtonsShowText() const
{
    for (const QToolButton *button : {m_runCommandButton, m_runEditorButton, m_inspectButton, m_clearButton}) {
        if (button != nullptr && !button->text().isEmpty()) {
            return true;
        }
    }
    return false;
}

bool PythonConsoleWidget::actionButtonsHaveIcons() const
{
    for (const QToolButton *button : {m_runCommandButton, m_runEditorButton, m_inspectButton, m_clearButton}) {
        if (button == nullptr || button->icon().isNull()) {
            return false;
        }
    }
    return true;
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

void PythonConsoleWidget::retranslateUi()
{
    if (m_output != nullptr && m_outputUsesDefaultMessage) {
        m_output->setPlainText(tr("Python Console ready. Use pyra or iface to access the embedded PyraQt API."));
    }
    if (m_editor != nullptr) {
        m_editor->setPlaceholderText(tr("Optional multi-line Python editor. Click Run Editor to execute in the shared interpreter."));
    }
    if (m_input != nullptr) {
        m_input->setPlaceholderText(tr(">>> Enter Python command"));
        m_input->setAccessibleName(tr("Python Console Input"));
        m_input->setAccessibleDescription(tr("Single-line Python command input"));
    }

    const auto configureButton = [](QToolButton *button, const QString &text, const QString &description) {
        if (button == nullptr) {
            return;
        }
        button->setText(QString());
        button->setToolTip(text);
        button->setStatusTip(text);
        button->setWhatsThis(description);
        button->setAccessibleName(text);
        button->setAccessibleDescription(description);
    };

    configureButton(m_runCommandButton, tr("Run Command"), tr("Executes the current single-line Python command"));
    configureButton(m_runEditorButton, tr("Run Editor"), tr("Executes the multi-line editor buffer in the shared interpreter"));
    configureButton(m_inspectButton, tr("Inspect"), tr("Lists the currently available Python global objects"));
    configureButton(m_clearButton, tr("Clear"), tr("Clears the console output"));
}

void PythonConsoleWidget::changeEvent(QEvent *event)
{
    if (event != nullptr && event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void PythonConsoleWidget::applyIcons()
{
    const QString themeName = m_themeManager == nullptr ? QStringLiteral("dark") : m_themeManager->currentTheme();
    if (m_runCommandButton != nullptr) {
        m_runCommandButton->setIcon(themedSvgIcon(QStringLiteral("run"), themeName, QSize(20, 20)));
    }
    if (m_runEditorButton != nullptr) {
        m_runEditorButton->setIcon(themedSvgIcon(QStringLiteral("save-all"), themeName, QSize(20, 20)));
    }
    if (m_inspectButton != nullptr) {
        m_inspectButton->setIcon(themedSvgIcon(QStringLiteral("inspect"), themeName, QSize(20, 20)));
    }
    if (m_clearButton != nullptr) {
        m_clearButton->setIcon(themedSvgIcon(QStringLiteral("clear-console"), themeName, QSize(20, 20)));
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
    clearOutput();
}

void PythonConsoleWidget::clearOutput()
{
    if (m_output != nullptr) {
        m_output->clear();
    }
    m_outputUsesDefaultMessage = false;
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

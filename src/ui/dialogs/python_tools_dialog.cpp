#include "ui/dialogs/python_tools_dialog.h"

#include "core/scripting/python_runtime_manager.h"
#include "core/scripting/script_execution_manager.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

namespace pyraqt::ui {

PythonToolsDialog::PythonToolsDialog(
    core::PythonRuntimeManager &runtimeManager,
    core::ScriptExecutionManager &executionManager,
    QWidget *parent)
    : QDialog(parent)
    , m_runtimeManager(runtimeManager)
    , m_executionManager(executionManager)
{
    setWindowTitle(tr("Python Tools"));
    resize(820, 620);

    auto *layout = new QVBoxLayout(this);
    auto *tabs = new QTabWidget(this);
    tabs->addTab(createMacrosPage(), tr("Macros"));
    tabs->addTab(createExpressionsPage(), tr("Expressions"));
    tabs->addTab(createProcessingPage(), tr("Processing"));
    layout->addWidget(tabs);

    m_output = new QPlainTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setMaximumHeight(130);
    layout->addWidget(m_output);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    layout->addWidget(buttons);

    connect(&m_runtimeManager, &core::PythonRuntimeManager::stdoutReceived, this, [this](const QString &output) {
        appendOutput(output.trimmed());
    });
}

QWidget *PythonToolsDialog::createMacrosPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel(tr("Project macros run in an isolated module and require macro execution to be enabled in Settings."), page));
    m_macroCodeEdit = new QTextEdit(page);
    m_macroCodeEdit->setPlainText(QStringLiteral("def openProject():\n    pyra.log.info('openProject macro ran')\n"));
    layout->addWidget(m_macroCodeEdit);

    auto *row = new QHBoxLayout();
    m_macroNameEdit = new QLineEdit(QStringLiteral("openProject"), page);
    auto *loadButton = new QPushButton(tr("Load Macro Code"), page);
    auto *triggerButton = new QPushButton(tr("Trigger Macro"), page);
    row->addWidget(m_macroNameEdit);
    row->addWidget(loadButton);
    row->addWidget(triggerButton);
    layout->addLayout(row);

    connect(loadButton, &QPushButton::clicked, this, [this] {
        const QString codeLiteral = QString::fromUtf8(QJsonDocument(QJsonArray{m_macroCodeEdit->toPlainText()}).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
        m_executionManager.runSelection(QStringLiteral("pyra.macros.load(%1)").arg(codeLiteral));
    });
    connect(triggerButton, &QPushButton::clicked, this, [this] {
        m_executionManager.runSelection(QStringLiteral("pyra.macros.trigger('%1')").arg(m_macroNameEdit->text()));
    });
    return page;
}

QWidget *PythonToolsDialog::createExpressionsPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    auto *form = new QFormLayout();
    m_expressionNameEdit = new QLineEdit(QStringLiteral("double_text"), page);
    m_expressionArgsEdit = new QLineEdit(QStringLiteral("ab"), page);
    form->addRow(tr("Function Name"), m_expressionNameEdit);
    form->addRow(tr("Arguments CSV"), m_expressionArgsEdit);
    layout->addLayout(form);
    m_expressionCodeEdit = new QTextEdit(page);
    m_expressionCodeEdit->setPlainText(QStringLiteral("def double_text(value):\n    return value + value\n"));
    layout->addWidget(m_expressionCodeEdit);
    auto *row = new QHBoxLayout();
    auto *registerButton = new QPushButton(tr("Register"), page);
    auto *evaluateButton = new QPushButton(tr("Evaluate"), page);
    row->addWidget(registerButton);
    row->addWidget(evaluateButton);
    layout->addLayout(row);
    connect(registerButton, &QPushButton::clicked, this, [this] {
        const QString codeLiteral = QString::fromUtf8(QJsonDocument(QJsonArray{m_expressionCodeEdit->toPlainText()}).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
        m_executionManager.runSelection(QStringLiteral("pyra.expressions.register('%1', %2)").arg(m_expressionNameEdit->text(), codeLiteral));
    });
    connect(evaluateButton, &QPushButton::clicked, this, [this] {
        const QStringList args = m_expressionArgsEdit->text().split(QLatin1Char(','), Qt::SkipEmptyParts);
        QStringList quoted;
        for (QString arg : args) {
            arg = arg.trimmed();
            arg.replace('\'', QStringLiteral("\\'"));
            quoted.push_back(QStringLiteral("'%1'").arg(arg));
        }
        m_executionManager.runSelection(QStringLiteral("pyra.expressions.evaluate('%1', [%2])").arg(m_expressionNameEdit->text(), quoted.join(QStringLiteral(", "))));
    });
    return page;
}

QWidget *PythonToolsDialog::createProcessingPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    auto *form = new QFormLayout();
    m_processingIdEdit = new QLineEdit(QStringLiteral("sample_algorithm"), page);
    m_processingParamsEdit = new QLineEdit(QStringLiteral("name=job"), page);
    form->addRow(tr("Algorithm ID"), m_processingIdEdit);
    form->addRow(tr("Parameters key=value CSV"), m_processingParamsEdit);
    layout->addLayout(form);
    m_processingCodeEdit = new QTextEdit(page);
    m_processingCodeEdit->setPlainText(QStringLiteral("def sample_algorithm(params):\n    return params.get('name', '') + '-done'\n"));
    layout->addWidget(m_processingCodeEdit);
    auto *row = new QHBoxLayout();
    auto *registerButton = new QPushButton(tr("Register"), page);
    auto *runButton = new QPushButton(tr("Run"), page);
    row->addWidget(registerButton);
    row->addWidget(runButton);
    layout->addLayout(row);
    connect(registerButton, &QPushButton::clicked, this, [this] {
        const QString codeLiteral = QString::fromUtf8(QJsonDocument(QJsonArray{m_processingCodeEdit->toPlainText()}).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
        m_executionManager.runSelection(QStringLiteral("pyra.processing.register('%1', %2)").arg(m_processingIdEdit->text(), codeLiteral));
    });
    connect(runButton, &QPushButton::clicked, this, [this] {
        QStringList entries;
        const QStringList pairs = m_processingParamsEdit->text().split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (QString pair : pairs) {
            const int separator = pair.indexOf(QLatin1Char('='));
            if (separator <= 0) {
                continue;
            }
            QString key = pair.left(separator).trimmed();
            QString value = pair.mid(separator + 1).trimmed();
            key.replace('\'', QStringLiteral("\\'"));
            value.replace('\'', QStringLiteral("\\'"));
            entries.push_back(QStringLiteral("'%1': '%2'").arg(key, value));
        }
        m_executionManager.runSelection(QStringLiteral("pyra.processing.run('%1', {%2})").arg(m_processingIdEdit->text(), entries.join(QStringLiteral(", "))));
    });
    return page;
}

void PythonToolsDialog::appendOutput(const QString &message)
{
    if (!message.isEmpty()) {
        m_output->appendPlainText(message);
    }
}

} // namespace pyraqt::ui

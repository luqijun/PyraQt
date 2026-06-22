#include "ui/dialogs/python_tools_dialog.h"

#include "core/scripting/python/python_runtime_manager.h"
#include "core/scripting/python/script_execution_manager.h"

#include <QEvent>
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
    resize(820, 620);

    auto *layout = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);
    m_tabs->addTab(createMacrosPage(), QString());
    m_tabs->addTab(createExpressionsPage(), QString());
    m_tabs->addTab(createProcessingPage(), QString());
    layout->addWidget(m_tabs);

    m_output = new QPlainTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setMaximumHeight(130);
    layout->addWidget(m_output);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
    layout->addWidget(m_buttonBox);

    connect(&m_runtimeManager, &core::PythonRuntimeManager::stdoutReceived, this, [this](const QString &output) {
        appendOutput(output.trimmed());
    });

    retranslateUi();
}

QWidget *PythonToolsDialog::createMacrosPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    m_macrosInfoLabel = new QLabel(page);
    layout->addWidget(m_macrosInfoLabel);
    m_macroCodeEdit = new QTextEdit(page);
    m_macroCodeEdit->setPlainText(QStringLiteral("def openProject():\n    pyra.log.info('openProject macro ran')\n"));
    layout->addWidget(m_macroCodeEdit);

    auto *row = new QHBoxLayout();
    m_macroNameEdit = new QLineEdit(QStringLiteral("openProject"), page);
    m_loadMacroButton = new QPushButton(page);
    m_triggerMacroButton = new QPushButton(page);
    row->addWidget(m_macroNameEdit);
    row->addWidget(m_loadMacroButton);
    row->addWidget(m_triggerMacroButton);
    layout->addLayout(row);

    connect(m_loadMacroButton, &QPushButton::clicked, this, [this] {
        const QString codeLiteral = QString::fromUtf8(QJsonDocument(QJsonArray{m_macroCodeEdit->toPlainText()}).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
        m_executionManager.runSelection(QStringLiteral("pyra.macros.load(%1)").arg(codeLiteral));
    });
    connect(m_triggerMacroButton, &QPushButton::clicked, this, [this] {
        m_executionManager.runSelection(QStringLiteral("pyra.macros.trigger('%1')").arg(m_macroNameEdit->text()));
    });
    return page;
}

QWidget *PythonToolsDialog::createExpressionsPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    m_expressionForm = new QFormLayout();
    m_expressionNameEdit = new QLineEdit(QStringLiteral("double_text"), page);
    m_expressionArgsEdit = new QLineEdit(QStringLiteral("ab"), page);
    m_expressionForm->addRow(QString(), m_expressionNameEdit);
    m_expressionForm->addRow(QString(), m_expressionArgsEdit);
    layout->addLayout(m_expressionForm);
    m_expressionCodeEdit = new QTextEdit(page);
    m_expressionCodeEdit->setPlainText(QStringLiteral("def double_text(value):\n    return value + value\n"));
    layout->addWidget(m_expressionCodeEdit);
    auto *row = new QHBoxLayout();
    m_registerExpressionButton = new QPushButton(page);
    m_evaluateExpressionButton = new QPushButton(page);
    row->addWidget(m_registerExpressionButton);
    row->addWidget(m_evaluateExpressionButton);
    layout->addLayout(row);
    connect(m_registerExpressionButton, &QPushButton::clicked, this, [this] {
        const QString codeLiteral = QString::fromUtf8(QJsonDocument(QJsonArray{m_expressionCodeEdit->toPlainText()}).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
        m_executionManager.runSelection(QStringLiteral("pyra.expressions.register('%1', %2)").arg(m_expressionNameEdit->text(), codeLiteral));
    });
    connect(m_evaluateExpressionButton, &QPushButton::clicked, this, [this] {
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
    m_processingForm = new QFormLayout();
    m_processingIdEdit = new QLineEdit(QStringLiteral("sample_algorithm"), page);
    m_processingParamsEdit = new QLineEdit(QStringLiteral("name=job"), page);
    m_processingForm->addRow(QString(), m_processingIdEdit);
    m_processingForm->addRow(QString(), m_processingParamsEdit);
    layout->addLayout(m_processingForm);
    m_processingCodeEdit = new QTextEdit(page);
    m_processingCodeEdit->setPlainText(QStringLiteral("def sample_algorithm(params):\n    return params.get('name', '') + '-done'\n"));
    layout->addWidget(m_processingCodeEdit);
    auto *row = new QHBoxLayout();
    m_registerProcessingButton = new QPushButton(page);
    m_runProcessingButton = new QPushButton(page);
    row->addWidget(m_registerProcessingButton);
    row->addWidget(m_runProcessingButton);
    layout->addLayout(row);
    connect(m_registerProcessingButton, &QPushButton::clicked, this, [this] {
        const QString codeLiteral = QString::fromUtf8(QJsonDocument(QJsonArray{m_processingCodeEdit->toPlainText()}).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
        m_executionManager.runSelection(QStringLiteral("pyra.processing.register('%1', %2)").arg(m_processingIdEdit->text(), codeLiteral));
    });
    connect(m_runProcessingButton, &QPushButton::clicked, this, [this] {
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

void PythonToolsDialog::changeEvent(QEvent *event)
{
    if (event != nullptr && event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(event);
}

void PythonToolsDialog::retranslateUi()
{
    setWindowTitle(tr("Python Tools"));
    if (m_tabs != nullptr) {
        m_tabs->setTabText(0, tr("Macros"));
        m_tabs->setTabText(1, tr("Expressions"));
        m_tabs->setTabText(2, tr("Processing"));
    }
    if (m_macrosInfoLabel != nullptr) {
        m_macrosInfoLabel->setText(tr("Project macros run in an isolated module and require macro execution to be enabled in Settings."));
    }
    if (m_loadMacroButton != nullptr) {
        m_loadMacroButton->setText(tr("Load Macro Code"));
    }
    if (m_triggerMacroButton != nullptr) {
        m_triggerMacroButton->setText(tr("Trigger Macro"));
    }
    if (m_expressionForm != nullptr) {
        m_expressionForm->setWidget(0, QFormLayout::LabelRole, new QLabel(tr("Function Name"), this));
        m_expressionForm->setWidget(1, QFormLayout::LabelRole, new QLabel(tr("Arguments CSV"), this));
    }
    if (m_registerExpressionButton != nullptr) {
        m_registerExpressionButton->setText(tr("Register"));
    }
    if (m_evaluateExpressionButton != nullptr) {
        m_evaluateExpressionButton->setText(tr("Evaluate"));
    }
    if (m_processingForm != nullptr) {
        m_processingForm->setWidget(0, QFormLayout::LabelRole, new QLabel(tr("Algorithm ID"), this));
        m_processingForm->setWidget(1, QFormLayout::LabelRole, new QLabel(tr("Parameters key=value CSV"), this));
    }
    if (m_registerProcessingButton != nullptr) {
        m_registerProcessingButton->setText(tr("Register"));
    }
    if (m_runProcessingButton != nullptr) {
        m_runProcessingButton->setText(tr("Run"));
    }
}

void PythonToolsDialog::appendOutput(const QString &message)
{
    if (!message.isEmpty()) {
        m_output->appendPlainText(message);
    }
}

} // namespace pyraqt::ui

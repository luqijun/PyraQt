#include "ui/dialogs/settings_dialog.h"

#include "core/config/config_manager.h"
#include "core/i18n/i18n_manager.h"
#include "core/scripting/python_runtime_manager.h"
#include "core/theme/theme_manager.h"
#include "core/update/update_manager.h"
#include "core/workspace/workspace_manager.h"
#include "ui/common/file_dialog_utils.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>

namespace pyraqt::ui {

SettingsDialog::SettingsDialog(
    core::ConfigManager &configManager,
    core::ThemeManager &themeManager,
    core::I18nManager &i18nManager,
    core::PythonRuntimeManager &pythonRuntimeManager,
    core::UpdateManager &updateManager,
    core::WorkspaceManager &workspaceManager,
    QWidget *parent)
    : QDialog(parent)
    , m_configManager(configManager)
    , m_themeManager(themeManager)
    , m_i18nManager(i18nManager)
    , m_pythonRuntimeManager(pythonRuntimeManager)
    , m_updateManager(updateManager)
    , m_workspaceManager(workspaceManager)
{
    createUi();
    populateValues();
}

void SettingsDialog::createUi()
{
    setWindowTitle(tr("Settings"));
    resize(760, 460);

    auto *layout = new QVBoxLayout(this);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search settings..."));
    m_searchEdit->setAccessibleName(tr("Settings Search"));
    m_searchEdit->setAccessibleDescription(tr("Filters available settings categories"));
    layout->addWidget(m_searchEdit);

    auto *contentLayout = new QHBoxLayout();
    m_categoryList = new QListWidget(this);
    m_categoryList->setAccessibleName(tr("Settings Categories"));
    m_categoryList->setAccessibleDescription(tr("List of settings categories"));
    m_categoryList->addItem(tr("General"));
    m_categoryList->addItem(tr("Python"));
    m_categoryList->addItem(tr("Updates"));
    m_pages = new QStackedWidget(this);
    m_pages->addWidget(createGeneralPage());
    m_pages->addWidget(createPythonPage());
    m_pages->addWidget(createUpdatesPage());
    contentLayout->addWidget(m_categoryList, 1);
    contentLayout->addWidget(m_pages, 3);
    layout->addLayout(contentLayout);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
    connect(m_categoryList, &QListWidget::currentRowChanged, m_pages, &QStackedWidget::setCurrentIndex);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SettingsDialog::applyFilter);
    m_categoryList->setCurrentRow(0);
}

void SettingsDialog::populateValues()
{
    m_themeCombo->setCurrentText(m_themeManager.currentTheme());
    m_languageCombo->setCurrentText(m_i18nManager.currentLocale());
    m_browserRootEdit->setText(m_workspaceManager.fileBrowserRoot());
    m_restoreSessionCheck->setChecked(m_workspaceManager.restoreLastSessionEnabled());
    m_interpreterEdit->setText(m_pythonRuntimeManager.interpreterPath());
    m_timeoutSpin->setValue(m_pythonRuntimeManager.executionTimeoutMs());
    m_macrosEnabledCheck->setChecked(m_pythonRuntimeManager.macrosEnabled());
    m_filesystemAccessCheck->setChecked(m_pythonRuntimeManager.fileSystemAccessEnabled());
    m_isolatedExecutionCheck->setChecked(m_pythonRuntimeManager.useIsolatedExecutionByDefault());
    m_consoleHistorySpin->setValue(m_pythonRuntimeManager.consoleHistoryLimit());
    m_codeCompletionCheck->setChecked(m_pythonRuntimeManager.codeCompletionEnabled());
    m_completionThresholdSpin->setValue(m_pythonRuntimeManager.completionTriggerThreshold());
    m_autoUpdateCheck->setChecked(m_updateManager.autoCheckEnabled());
    m_updateChannelCombo->setCurrentText(m_updateManager.currentChannel());
}

void SettingsDialog::applyFilter(const QString &text)
{
    const QString needle = text.trimmed();
    for (int index = 0; index < m_categoryList->count(); ++index) {
        const bool visible = needle.isEmpty() || m_categoryList->item(index)->text().contains(needle, Qt::CaseInsensitive);
        m_categoryList->item(index)->setHidden(!visible);
    }
}

QWidget *SettingsDialog::createGeneralPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QFormLayout(page);

    m_themeCombo = new QComboBox(page);
    m_themeCombo->addItems(m_themeManager.availableThemes());
    m_themeCombo->setAccessibleName(tr("Theme"));

    m_languageCombo = new QComboBox(page);
    m_languageCombo->addItems(m_i18nManager.availableLocales());
    m_languageCombo->setAccessibleName(tr("Language"));

    auto *browserRootContainer = new QWidget(page);
    auto *browserRootLayout = new QHBoxLayout(browserRootContainer);
    browserRootLayout->setContentsMargins(0, 0, 0, 0);
    m_browserRootEdit = new QLineEdit(browserRootContainer);
    auto *browseButton = new QPushButton(tr("Browse"), browserRootContainer);
    browserRootLayout->addWidget(m_browserRootEdit);
    browserRootLayout->addWidget(browseButton);

    m_restoreSessionCheck = new QCheckBox(tr("Restore previous session on startup"), page);

    layout->addRow(tr("Theme"), m_themeCombo);
    layout->addRow(tr("Language"), m_languageCombo);
    layout->addRow(tr("File Browser Root"), browserRootContainer);
    layout->addRow(QString(), m_restoreSessionCheck);

    connect(m_themeCombo, &QComboBox::currentTextChanged, &m_themeManager, &core::ThemeManager::setTheme);
    connect(m_languageCombo, &QComboBox::currentTextChanged, &m_i18nManager, &core::I18nManager::setLocale);
    connect(m_restoreSessionCheck, &QCheckBox::toggled, &m_workspaceManager, &core::WorkspaceManager::setRestoreLastSessionEnabled);
    connect(browseButton, &QPushButton::clicked, this, [this] {
        const QString directory = getThemedExistingDirectory(
            {
                tr("Choose File Browser Root"),
                m_browserRootEdit->text(),
                QString(),
                QFileDialog::Directory,
                QFileDialog::AcceptOpen,
            },
            this);
        if (!directory.isEmpty()) {
            m_browserRootEdit->setText(directory);
            m_workspaceManager.setFileBrowserRoot(directory);
            const bool saved = m_configManager.save();
            Q_UNUSED(saved)
        }
    });
    connect(m_browserRootEdit, &QLineEdit::editingFinished, this, [this] {
        m_workspaceManager.setFileBrowserRoot(m_browserRootEdit->text());
        const bool saved = m_configManager.save();
        Q_UNUSED(saved)
    });

    return page;
}

QWidget *SettingsDialog::createPythonPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QFormLayout(page);

    auto *interpreterContainer = new QWidget(page);
    auto *interpreterLayout = new QHBoxLayout(interpreterContainer);
    interpreterLayout->setContentsMargins(0, 0, 0, 0);
    m_interpreterEdit = new QLineEdit(interpreterContainer);
    auto *browseButton = new QPushButton(tr("Browse"), interpreterContainer);
    interpreterLayout->addWidget(m_interpreterEdit);
    interpreterLayout->addWidget(browseButton);

    m_timeoutSpin = new QSpinBox(page);
    m_timeoutSpin->setRange(1000, 300000);
    m_timeoutSpin->setSingleStep(1000);
    m_timeoutSpin->setSuffix(tr(" ms"));
    m_consoleHistorySpin = new QSpinBox(page);
    m_consoleHistorySpin->setRange(10, 5000);
    m_consoleHistorySpin->setSingleStep(10);
    m_completionThresholdSpin = new QSpinBox(page);
    m_completionThresholdSpin->setRange(1, 8);
    m_completionThresholdSpin->setSingleStep(1);
    m_codeCompletionCheck = new QCheckBox(tr("Enable Python code completion"), page);
    m_macrosEnabledCheck = new QCheckBox(tr("Allow project macros to run"), page);
    m_filesystemAccessCheck = new QCheckBox(tr("Allow Python file system helpers"), page);
    m_isolatedExecutionCheck = new QCheckBox(tr("Run scripts in isolated subprocess by default"), page);

    layout->addRow(tr("Interpreter"), interpreterContainer);
    layout->addRow(tr("Execution Timeout"), m_timeoutSpin);
    layout->addRow(tr("Console History"), m_consoleHistorySpin);
    layout->addRow(tr("Completion Trigger"), m_completionThresholdSpin);
    layout->addRow(QString(), m_codeCompletionCheck);
    layout->addRow(QString(), m_macrosEnabledCheck);
    layout->addRow(QString(), m_filesystemAccessCheck);
    layout->addRow(QString(), m_isolatedExecutionCheck);
    layout->addRow(new QLabel(tr("Embedded Python is initialized in-process; isolated mode remains available for long-running scripts."), page));

    connect(m_interpreterEdit, &QLineEdit::editingFinished, this, [this] {
        m_pythonRuntimeManager.setInterpreterPath(m_interpreterEdit->text());
    });
    connect(m_timeoutSpin, qOverload<int>(&QSpinBox::valueChanged), &m_pythonRuntimeManager, &core::PythonRuntimeManager::setExecutionTimeoutMs);
    connect(m_macrosEnabledCheck, &QCheckBox::toggled, &m_pythonRuntimeManager, &core::PythonRuntimeManager::setMacrosEnabled);
    connect(m_filesystemAccessCheck, &QCheckBox::toggled, &m_pythonRuntimeManager, &core::PythonRuntimeManager::setFileSystemAccessEnabled);
    connect(m_isolatedExecutionCheck, &QCheckBox::toggled, &m_pythonRuntimeManager, &core::PythonRuntimeManager::setUseIsolatedExecutionByDefault);
    connect(m_consoleHistorySpin, qOverload<int>(&QSpinBox::valueChanged), &m_pythonRuntimeManager, &core::PythonRuntimeManager::setConsoleHistoryLimit);
    connect(m_codeCompletionCheck, &QCheckBox::toggled, &m_pythonRuntimeManager, &core::PythonRuntimeManager::setCodeCompletionEnabled);
    connect(m_completionThresholdSpin, qOverload<int>(&QSpinBox::valueChanged), &m_pythonRuntimeManager, &core::PythonRuntimeManager::setCompletionTriggerThreshold);
    connect(browseButton, &QPushButton::clicked, this, [this] {
        const QString filePath = getThemedOpenFileName(
            {
                tr("Choose Python Interpreter"),
                m_interpreterEdit->text(),
                QString(),
                QFileDialog::ExistingFile,
                QFileDialog::AcceptOpen,
            },
            this);
        if (!filePath.isEmpty()) {
            m_interpreterEdit->setText(filePath);
            m_pythonRuntimeManager.setInterpreterPath(filePath);
        }
    });

    return page;
}

QWidget *SettingsDialog::createUpdatesPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QFormLayout(page);

    m_autoUpdateCheck = new QCheckBox(tr("Automatically check for updates"), page);
    m_updateChannelCombo = new QComboBox(page);
    m_updateChannelCombo->addItems({QStringLiteral("stable"), QStringLiteral("preview")});

    layout->addRow(QString(), m_autoUpdateCheck);
    layout->addRow(tr("Update Channel"), m_updateChannelCombo);
    layout->addRow(new QLabel(tr("Update installation remains unavailable in this build."), page));

    connect(m_autoUpdateCheck, &QCheckBox::toggled, &m_updateManager, &core::UpdateManager::setAutoCheckEnabled);
    connect(m_updateChannelCombo, &QComboBox::currentTextChanged, &m_updateManager, &core::UpdateManager::setChannel);

    return page;
}

} // namespace pyraqt::ui

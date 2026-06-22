#include "ui/dialogs/settings_dialog.h"

#include "core/config/config_manager.h"
#include "core/i18n/i18n_manager.h"
#include "core/scripting/python/python_runtime_manager.h"
#include "core/theme/theme_manager.h"
#include "core/update/update_manager.h"
#include "core/workspace/workspace_manager.h"
#include "ui/common/file_dialog_utils.h"

#include <QEvent>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QFileInfo>
#include <QStackedWidget>
#include <QStandardPaths>
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
    resize(760, 460);

    auto *layout = new QVBoxLayout(this);
    m_searchEdit = new QLineEdit(this);
    layout->addWidget(m_searchEdit);

    auto *contentLayout = new QHBoxLayout();
    m_categoryList = new QListWidget(this);
    m_categoryList->addItem(QString());
    m_categoryList->addItem(QString());
    m_categoryList->addItem(QString());
    m_pages = new QStackedWidget(this);
    m_pages->addWidget(createGeneralPage());
    m_pages->addWidget(createPythonPage());
    m_pages->addWidget(createUpdatesPage());
    contentLayout->addWidget(m_categoryList, 1);
    contentLayout->addWidget(m_pages, 3);
    layout->addLayout(contentLayout);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    layout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
    connect(m_categoryList, &QListWidget::currentRowChanged, m_pages, &QStackedWidget::setCurrentIndex);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SettingsDialog::applyFilter);
    m_categoryList->setCurrentRow(0);
    retranslateUi();
}

void SettingsDialog::populateValues()
{
    m_themeCombo->setCurrentText(m_themeManager.currentTheme());
    m_languageCombo->setCurrentText(m_i18nManager.currentLocale());
    m_browserRootEdit->setText(m_workspaceManager.fileBrowserRoot());
    m_restoreSessionCheck->setChecked(m_workspaceManager.restoreLastSessionEnabled());
    refreshInterpreterPathDisplay();
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
    m_generalFormLayout = new QFormLayout(page);

    m_themeCombo = new QComboBox(page);
    m_themeCombo->addItems(m_themeManager.availableThemes());

    m_languageCombo = new QComboBox(page);
    m_languageCombo->addItems(m_i18nManager.availableLocales());

    auto *browserRootContainer = new QWidget(page);
    auto *browserRootLayout = new QHBoxLayout(browserRootContainer);
    browserRootLayout->setContentsMargins(0, 0, 0, 0);
    m_browserRootEdit = new QLineEdit(browserRootContainer);
    m_browserRootBrowseButton = new QPushButton(browserRootContainer);
    browserRootLayout->addWidget(m_browserRootEdit);
    browserRootLayout->addWidget(m_browserRootBrowseButton);

    m_restoreSessionCheck = new QCheckBox(page);

    m_generalFormLayout->addRow(QString(), m_themeCombo);
    m_generalFormLayout->addRow(QString(), m_languageCombo);
    m_generalFormLayout->addRow(QString(), browserRootContainer);
    m_generalFormLayout->addRow(QString(), m_restoreSessionCheck);

    connect(m_themeCombo, &QComboBox::currentTextChanged, &m_themeManager, &core::ThemeManager::setTheme);
    connect(m_languageCombo, &QComboBox::currentTextChanged, &m_i18nManager, &core::I18nManager::setLocale);
    connect(m_restoreSessionCheck, &QCheckBox::toggled, &m_workspaceManager, &core::WorkspaceManager::setRestoreLastSessionEnabled);
    connect(m_browserRootBrowseButton, &QPushButton::clicked, this, [this] {
        const QString directory = getThemedExistingDirectory(
            {
                tr("Choose File Browser Root"),
                m_browserRootEdit->text(),
                QString(),
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
    m_pythonFormLayout = new QFormLayout(page);

    auto *interpreterContainer = new QWidget(page);
    auto *interpreterLayout = new QHBoxLayout(interpreterContainer);
    interpreterLayout->setContentsMargins(0, 0, 0, 0);
    m_interpreterEdit = new QLineEdit(interpreterContainer);
    m_interpreterBrowseButton = new QPushButton(interpreterContainer);
    interpreterLayout->addWidget(m_interpreterEdit);
    interpreterLayout->addWidget(m_interpreterBrowseButton);

    m_timeoutSpin = new QSpinBox(page);
    m_timeoutSpin->setRange(1000, 300000);
    m_timeoutSpin->setSingleStep(1000);
    m_consoleHistorySpin = new QSpinBox(page);
    m_consoleHistorySpin->setRange(10, 5000);
    m_consoleHistorySpin->setSingleStep(10);
    m_completionThresholdSpin = new QSpinBox(page);
    m_completionThresholdSpin->setRange(1, 8);
    m_completionThresholdSpin->setSingleStep(1);
    m_codeCompletionCheck = new QCheckBox(page);
    m_macrosEnabledCheck = new QCheckBox(page);
    m_filesystemAccessCheck = new QCheckBox(page);
    m_isolatedExecutionCheck = new QCheckBox(page);

    m_pythonFormLayout->addRow(QString(), interpreterContainer);
    m_pythonFormLayout->addRow(QString(), m_timeoutSpin);
    m_pythonFormLayout->addRow(QString(), m_consoleHistorySpin);
    m_pythonFormLayout->addRow(QString(), m_completionThresholdSpin);
    m_pythonFormLayout->addRow(QString(), m_codeCompletionCheck);
    m_pythonFormLayout->addRow(QString(), m_macrosEnabledCheck);
    m_pythonFormLayout->addRow(QString(), m_filesystemAccessCheck);
    m_pythonFormLayout->addRow(QString(), m_isolatedExecutionCheck);
    m_pythonInfoLabel = new QLabel(page);
    m_pythonFormLayout->addRow(m_pythonInfoLabel);

    connect(m_interpreterEdit, &QLineEdit::editingFinished, this, [this] {
        m_pythonRuntimeManager.setInterpreterPath(m_interpreterEdit->text());
        refreshInterpreterPathDisplay();
    });
    connect(m_timeoutSpin, qOverload<int>(&QSpinBox::valueChanged), &m_pythonRuntimeManager, &core::PythonRuntimeManager::setExecutionTimeoutMs);
    connect(m_macrosEnabledCheck, &QCheckBox::toggled, &m_pythonRuntimeManager, &core::PythonRuntimeManager::setMacrosEnabled);
    connect(m_filesystemAccessCheck, &QCheckBox::toggled, &m_pythonRuntimeManager, &core::PythonRuntimeManager::setFileSystemAccessEnabled);
    connect(m_isolatedExecutionCheck, &QCheckBox::toggled, &m_pythonRuntimeManager, &core::PythonRuntimeManager::setUseIsolatedExecutionByDefault);
    connect(m_consoleHistorySpin, qOverload<int>(&QSpinBox::valueChanged), &m_pythonRuntimeManager, &core::PythonRuntimeManager::setConsoleHistoryLimit);
    connect(m_codeCompletionCheck, &QCheckBox::toggled, &m_pythonRuntimeManager, &core::PythonRuntimeManager::setCodeCompletionEnabled);
    connect(m_completionThresholdSpin, qOverload<int>(&QSpinBox::valueChanged), &m_pythonRuntimeManager, &core::PythonRuntimeManager::setCompletionTriggerThreshold);
    connect(m_interpreterBrowseButton, &QPushButton::clicked, this, [this] {
        const QString filePath = getThemedOpenFileName(
            {
                tr("Choose Python Interpreter"),
                m_interpreterEdit->text(),
                QString(),
                QString(),
                QFileDialog::ExistingFile,
                QFileDialog::AcceptOpen,
            },
            this);
        if (!filePath.isEmpty()) {
            m_interpreterEdit->setText(filePath);
            m_pythonRuntimeManager.setInterpreterPath(filePath);
            refreshInterpreterPathDisplay();
        }
    });

    return page;
}

void SettingsDialog::refreshInterpreterPathDisplay()
{
    if (m_interpreterEdit == nullptr) {
        return;
    }

    const QString configuredPath = m_pythonRuntimeManager.interpreterPath();
    QString displayPath = configuredPath;
    const QString resolvedPath = QStandardPaths::findExecutable(configuredPath);
    if (!resolvedPath.isEmpty()) {
        displayPath = QFileInfo(resolvedPath).absoluteFilePath();
    } else if (QFileInfo(configuredPath).exists()) {
        displayPath = QFileInfo(configuredPath).absoluteFilePath();
    }

    m_interpreterEdit->setText(displayPath);
    m_interpreterEdit->setCursorPosition(0);
    m_interpreterEdit->setToolTip(displayPath);
}

QWidget *SettingsDialog::createUpdatesPage()
{
    auto *page = new QWidget(this);
    m_updatesFormLayout = new QFormLayout(page);

    m_autoUpdateCheck = new QCheckBox(page);
    m_updateChannelCombo = new QComboBox(page);
    m_updateChannelCombo->addItems({QStringLiteral("stable"), QStringLiteral("preview")});
    m_updatesInfoLabel = new QLabel(page);

    m_updatesFormLayout->addRow(QString(), m_autoUpdateCheck);
    m_updatesFormLayout->addRow(QString(), m_updateChannelCombo);
    m_updatesFormLayout->addRow(m_updatesInfoLabel);

    connect(m_autoUpdateCheck, &QCheckBox::toggled, &m_updateManager, &core::UpdateManager::setAutoCheckEnabled);
    connect(m_updateChannelCombo, &QComboBox::currentTextChanged, &m_updateManager, &core::UpdateManager::setChannel);

    return page;
}

void SettingsDialog::changeEvent(QEvent *event)
{
    if (event != nullptr && event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(event);
}

void SettingsDialog::retranslateUi()
{
    setWindowTitle(tr("Settings"));
    if (m_searchEdit != nullptr) {
        m_searchEdit->setPlaceholderText(tr("Search settings..."));
        m_searchEdit->setAccessibleName(tr("Settings Search"));
        m_searchEdit->setAccessibleDescription(tr("Filters available settings categories"));
    }
    if (m_categoryList != nullptr) {
        m_categoryList->setAccessibleName(tr("Settings Categories"));
        m_categoryList->setAccessibleDescription(tr("List of settings categories"));
        if (m_categoryList->count() > 0) {
            m_categoryList->item(0)->setText(tr("General"));
        }
        if (m_categoryList->count() > 1) {
            m_categoryList->item(1)->setText(tr("Python"));
        }
        if (m_categoryList->count() > 2) {
            m_categoryList->item(2)->setText(tr("Updates"));
        }
    }
    if (m_themeCombo != nullptr) {
        m_themeCombo->setAccessibleName(tr("Theme"));
    }
    if (m_languageCombo != nullptr) {
        m_languageCombo->setAccessibleName(tr("Language"));
    }
    if (m_browserRootBrowseButton != nullptr) {
        m_browserRootBrowseButton->setText(tr("Browse"));
    }
    if (m_restoreSessionCheck != nullptr) {
        m_restoreSessionCheck->setText(tr("Restore previous session on startup"));
    }
    if (m_generalFormLayout != nullptr) {
        m_generalFormLayout->setWidget(0, QFormLayout::LabelRole, new QLabel(tr("Theme"), this));
        m_generalFormLayout->setWidget(1, QFormLayout::LabelRole, new QLabel(tr("Language"), this));
        m_generalFormLayout->setWidget(2, QFormLayout::LabelRole, new QLabel(tr("File Browser Root"), this));
    }
    if (m_interpreterBrowseButton != nullptr) {
        m_interpreterBrowseButton->setText(tr("Browse"));
    }
    if (m_timeoutSpin != nullptr) {
        m_timeoutSpin->setSuffix(tr(" ms"));
    }
    if (m_codeCompletionCheck != nullptr) {
        m_codeCompletionCheck->setText(tr("Enable Python code completion"));
    }
    if (m_macrosEnabledCheck != nullptr) {
        m_macrosEnabledCheck->setText(tr("Allow project macros to run"));
    }
    if (m_filesystemAccessCheck != nullptr) {
        m_filesystemAccessCheck->setText(tr("Allow Python file system helpers"));
    }
    if (m_isolatedExecutionCheck != nullptr) {
        m_isolatedExecutionCheck->setText(tr("Run scripts in isolated subprocess by default"));
    }
    if (m_pythonInfoLabel != nullptr) {
        m_pythonInfoLabel->setText(tr("Embedded Python is initialized in-process; isolated mode remains available for long-running scripts."));
    }
    if (m_pythonFormLayout != nullptr) {
        m_pythonFormLayout->setWidget(0, QFormLayout::LabelRole, new QLabel(tr("Interpreter"), this));
        m_pythonFormLayout->setWidget(1, QFormLayout::LabelRole, new QLabel(tr("Execution Timeout"), this));
        m_pythonFormLayout->setWidget(2, QFormLayout::LabelRole, new QLabel(tr("Console History"), this));
        m_pythonFormLayout->setWidget(3, QFormLayout::LabelRole, new QLabel(tr("Completion Trigger"), this));
    }
    if (m_autoUpdateCheck != nullptr) {
        m_autoUpdateCheck->setText(tr("Automatically check for updates"));
    }
    if (m_updatesInfoLabel != nullptr) {
        m_updatesInfoLabel->setText(tr("Update installation remains unavailable in this build."));
    }
    if (m_updatesFormLayout != nullptr) {
        m_updatesFormLayout->setWidget(1, QFormLayout::LabelRole, new QLabel(tr("Update Channel"), this));
    }
}

} // namespace pyraqt::ui

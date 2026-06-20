#include "ui/mainwindow/main_window.h"

#include "core/command/command_descriptor.h"
#include "core/command/command_manager.h"
#include "core/config/config_manager.h"
#include "core/i18n/i18n_manager.h"
#include "core/logging/log_manager.h"
#include "core/plugin/plugin_manager.h"
#include "core/runtime/crash_recovery_manager.h"
#include "core/scripting/python_runtime_manager.h"
#include "core/scripting/script_execution_manager.h"
#include "core/theme/theme_manager.h"
#include "core/update/update_manager.h"
#include "core/update/update_types.h"
#include "core/workspace/workspace_manager.h"
#include "ui/dialogs/command_palette_dialog.h"
#include "ui/dialogs/settings_dialog.h"
#include "ui/common/file_dialog_utils.h"
#include "ui/editor/editor_workspace_widget.h"
#include "ui/editor/script_editor_widget.h"
#include "ui/panels/files/file_browser_panel.h"
#include "ui/panels/plugins/plugin_manager_panel.h"

#include <QAction>
#include <QDockWidget>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSettings>
#include <QStatusBar>
#include <QToolBar>

namespace pyraqt::ui {

MainWindow::MainWindow(
    core::ConfigManager &configManager,
    core::LogManager &logManager,
    core::ThemeManager &themeManager,
    core::I18nManager &i18nManager,
    core::PythonRuntimeManager &pythonRuntimeManager,
    core::ScriptExecutionManager &scriptExecutionManager,
    core::CommandManager &commandManager,
    core::PluginManager &pluginManager,
    core::WorkspaceManager &workspaceManager,
    core::UpdateManager &updateManager,
    core::CrashRecoveryManager &crashRecoveryManager,
    QWidget *parent)
    : QMainWindow(parent)
    , m_configManager(configManager)
    , m_logManager(logManager)
    , m_themeManager(themeManager)
    , m_i18nManager(i18nManager)
    , m_pythonRuntimeManager(pythonRuntimeManager)
    , m_scriptExecutionManager(scriptExecutionManager)
    , m_commandManager(commandManager)
    , m_pluginManager(pluginManager)
    , m_workspaceManager(workspaceManager)
    , m_updateManager(updateManager)
    , m_crashRecoveryManager(crashRecoveryManager)
{
    setObjectName(QStringLiteral("mainWindow"));
    resize(1280, 800);

    createCentralEditor();
    createDocks();
    createMenus();
    createStatusBar();
    registerBuiltInCommands();
    restoreSession();
    refreshRecentFilesMenu();
    retranslateUi();
    showRecoveryPromptIfNeeded();

    connect(&m_logManager, &core::LogManager::messageLogged, this, [this](const QString &level, const QString &message) {
        appendConsoleLine(level, message);
    });
    connect(&m_workspaceManager, &core::WorkspaceManager::recentFilesChanged, this, [this](const QStringList &) {
        refreshRecentFilesMenu();
    });
    connect(&m_workspaceManager, &core::WorkspaceManager::fileBrowserRootChanged, this, [this](const QString &path) {
        if (m_fileBrowserPanel != nullptr) {
            m_fileBrowserPanel->setRootPath(path);
        }
    });
    connect(&m_i18nManager, &core::I18nManager::localeChanged, this, [this] {
        retranslateUi();
        refreshRecentFilesMenu();
    });
    connect(&m_pythonRuntimeManager, &core::PythonRuntimeManager::interpreterChanged, this, [this](const QString &) {
        if (m_pythonStatusLabel != nullptr) {
            m_pythonStatusLabel->setText(tr("Python: %1").arg(m_pythonRuntimeManager.pythonVersion()));
        }
    });

    connect(&m_scriptExecutionManager, &core::ScriptExecutionManager::stdoutReceived, this, [this](const QString &output) {
        appendConsoleLine(QStringLiteral("stdout"), output.trimmed());
    });
    connect(&m_scriptExecutionManager, &core::ScriptExecutionManager::stderrReceived, this, [this](const QString &output) {
        appendConsoleLine(QStringLiteral("stderr"), output.trimmed());
    });
    connect(&m_scriptExecutionManager, &core::ScriptExecutionManager::executionStarted, this, [this](const QString &label) {
        statusBar()->showMessage(tr("Running %1").arg(label));
        if (m_stopScriptAction != nullptr) {
            m_stopScriptAction->setEnabled(true);
        }
    });
    connect(&m_scriptExecutionManager, &core::ScriptExecutionManager::executionFinished, this, [this](int exitCode) {
        statusBar()->showMessage(tr("Execution finished with code %1").arg(exitCode));
        if (m_stopScriptAction != nullptr) {
            m_stopScriptAction->setEnabled(false);
        }
    });
    connect(&m_scriptExecutionManager, &core::ScriptExecutionManager::executionFailed, this, [this](const QString &message) {
        appendConsoleLine(QStringLiteral("error"), message);
        statusBar()->showMessage(tr("Execution failed"));
        if (m_stopScriptAction != nullptr) {
            m_stopScriptAction->setEnabled(false);
        }
    });
    connect(&m_scriptExecutionManager, &core::ScriptExecutionManager::statusMessageRequested, this, [this](const QString &message) {
        statusBar()->showMessage(message);
    });
    connect(&m_scriptExecutionManager, &core::ScriptExecutionManager::dialogMessageRequested, this, [this](const QString &message) {
        QMessageBox::information(this, tr("Script Message"), message);
    });
    connect(&m_scriptExecutionManager, &core::ScriptExecutionManager::logMessageRequested, this,
        [this](const QString &level, const QString &message) {
            appendConsoleLine(level, message);
            if (level == QStringLiteral("error")) {
                m_logManager.error(message);
            } else if (level == QStringLiteral("warn")) {
                m_logManager.warning(message);
            } else {
                m_logManager.info(message);
            }
        });
}

void MainWindow::saveSession()
{
    QSettings settings(QStringLiteral("PyraQt"), QStringLiteral("PyraQt"));
    settings.setValue(QStringLiteral("mainWindow/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("mainWindow/state"), saveState());

    if (m_workspaceWidget != nullptr) {
        m_workspaceManager.saveSession(
            m_workspaceWidget->captureSession(m_workspaceManager.recentFiles(),
                m_fileBrowserPanel != nullptr ? m_fileBrowserPanel->rootPath() : m_workspaceManager.fileBrowserRoot()));
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::createCentralEditor()
{
    m_workspaceWidget = new EditorWorkspaceWidget(m_themeManager, this);
    m_workspaceWidget->setObjectName(QStringLiteral("editorWorkspace"));
    setCentralWidget(m_workspaceWidget);

    connect(m_workspaceWidget, &EditorWorkspaceWidget::documentModificationChanged, this, [this](bool modified) {
        if (m_documentStatusLabel != nullptr) {
            m_documentStatusLabel->setText(modified ? tr("Document: Modified") : tr("Document: Saved"));
        }
    });
    connect(m_workspaceWidget, &EditorWorkspaceWidget::currentFilePathChanged, this, [this](const QString &filePath) {
        if (m_fileStatusLabel != nullptr) {
            m_fileStatusLabel->setText(filePath.isEmpty() ? tr("File: Untitled") : tr("File: %1").arg(QFileInfo(filePath).fileName()));
        }
        if (filePath.isEmpty()) {
            setWindowTitle(tr("PyraQt - Untitled Script"));
        } else {
            setWindowTitle(tr("PyraQt - %1").arg(filePath));
            m_configManager.setValue(QStringLiteral("python.last_script_path"), filePath);
            m_configManager.setValue(QStringLiteral("python.last_directory"), QFileInfo(filePath).absolutePath());
            const bool saved = m_configManager.save();
            if (!saved) {
                m_logManager.warning(tr("Failed to save script path metadata."));
            }
            m_workspaceManager.addRecentFile(filePath);
        }
    });
    connect(m_workspaceWidget, &EditorWorkspaceWidget::currentCursorChanged, this, [this](int line, int column) {
        if (m_cursorStatusLabel != nullptr) {
            m_cursorStatusLabel->setText(tr("Ln %1, Col %2").arg(line).arg(column));
        }
    });
    connect(m_workspaceWidget, &EditorWorkspaceWidget::requestCloseConfirmation, this,
        [this](const QString &title, const QString &message, bool *accepted) {
            if (accepted == nullptr) {
                return;
            }
            *accepted = QMessageBox::question(this, title, message, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
        });
}

void MainWindow::createDocks()
{
    auto *fileBrowserDock = new QDockWidget(tr("File Browser"), this);
    fileBrowserDock->setObjectName(QStringLiteral("fileBrowserDock"));
    m_fileBrowserPanel = new FileBrowserPanel(fileBrowserDock);
    fileBrowserDock->setWidget(m_fileBrowserPanel);
    addDockWidget(Qt::LeftDockWidgetArea, fileBrowserDock);

    connect(m_fileBrowserPanel, &FileBrowserPanel::fileActivated, this, [this](const QString &path) {
        if (m_workspaceWidget->openFile(path)) {
            m_workspaceManager.addRecentFile(path);
        }
    });

    auto *pluginDock = new QDockWidget(tr("Plugins"), this);
    pluginDock->setObjectName(QStringLiteral("pluginDock"));
    m_pluginManagerPanel = new PluginManagerPanel(m_pluginManager, pluginDock);
    pluginDock->setWidget(m_pluginManagerPanel);
    addDockWidget(Qt::LeftDockWidgetArea, pluginDock);

    addDockWidget(Qt::RightDockWidgetArea,
        createTextDock(QStringLiteral("propertiesDock"), tr("Properties"), tr("Selected item properties will appear here.")));

    auto *consoleDock = new QDockWidget(tr("Console"), this);
    consoleDock->setObjectName(QStringLiteral("consoleDock"));
    m_console = new QPlainTextEdit(consoleDock);
    m_console->setReadOnly(true);
    m_console->setPlainText(tr("Application console output will appear here."));
    consoleDock->setWidget(m_console);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    addDockWidget(Qt::BottomDockWidgetArea,
        createTextDock(QStringLiteral("logViewerDock"), tr("Log Viewer"), tr("Structured logs are mirrored to this area.")));
}

void MainWindow::createMenus()
{
    auto *fileMenu = menuBar()->addMenu(QString());
    fileMenu->setObjectName(QStringLiteral("fileMenu"));
    createScriptActions();
    fileMenu->addAction(m_newScriptAction);
    fileMenu->addAction(m_openScriptAction);
    m_recentFilesMenu = fileMenu->addMenu(QString());
    m_recentFilesMenu->setObjectName(QStringLiteral("recentFilesMenu"));
    fileMenu->addAction(m_saveScriptAction);
    fileMenu->addAction(m_saveAllScriptsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_closeTabAction);
    fileMenu->addAction(m_closeOtherTabsAction);
    fileMenu->addAction(m_reopenSessionAction);
    fileMenu->addSeparator();
    auto *quitAction = fileMenu->addAction(QString());
    quitAction->setObjectName(QStringLiteral("quitAction"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    auto *viewMenu = menuBar()->addMenu(QString());
    viewMenu->setObjectName(QStringLiteral("viewMenu"));

    auto *toolsMenu = menuBar()->addMenu(QString());
    toolsMenu->setObjectName(QStringLiteral("toolsMenu"));
    toolsMenu->addAction(m_commandPaletteAction);
    toolsMenu->addAction(m_settingsAction);
    toolsMenu->addAction(m_chooseFileBrowserRootAction);
    toolsMenu->addAction(m_checkUpdatesAction);

    auto *themeMenu = viewMenu->addMenu(QString());
    themeMenu->setObjectName(QStringLiteral("themeMenu"));
    m_lightThemeAction = themeMenu->addAction(QString());
    m_darkThemeAction = themeMenu->addAction(QString());
    connect(m_lightThemeAction, &QAction::triggered, &m_themeManager, &core::ThemeManager::setLightTheme);
    connect(m_darkThemeAction, &QAction::triggered, &m_themeManager, &core::ThemeManager::setDarkTheme);

    auto *languageMenu = viewMenu->addMenu(QString());
    languageMenu->setObjectName(QStringLiteral("languageMenu"));
    m_englishAction = languageMenu->addAction(QString());
    m_chineseAction = languageMenu->addAction(QString());
    connect(m_englishAction, &QAction::triggered, this, [this] {
        m_i18nManager.setLocale(QStringLiteral("en_US"));
    });
    connect(m_chineseAction, &QAction::triggered, this, [this] {
        m_i18nManager.setLocale(QStringLiteral("zh_CN"));
    });

    auto *mainToolbar = addToolBar(QString());
    mainToolbar->setObjectName(QStringLiteral("mainToolBar"));
    mainToolbar->addAction(m_newScriptAction);
    mainToolbar->addAction(m_openScriptAction);
    mainToolbar->addAction(m_saveScriptAction);
    mainToolbar->addAction(m_saveAllScriptsAction);
    mainToolbar->addAction(m_runScriptAction);
    mainToolbar->addAction(m_stopScriptAction);
    mainToolbar->addAction(m_commandPaletteAction);
    mainToolbar->addAction(m_settingsAction);
    mainToolbar->addAction(m_checkUpdatesAction);
    mainToolbar->addAction(m_lightThemeAction);
    mainToolbar->addAction(m_darkThemeAction);
}

void MainWindow::createStatusBar()
{
    m_pythonStatusLabel = new QLabel(tr("Python: %1").arg(m_pythonRuntimeManager.pythonVersion()), this);
    m_documentStatusLabel = new QLabel(tr("Document: Saved"), this);
    m_fileStatusLabel = new QLabel(tr("File: Untitled"), this);
    m_cursorStatusLabel = new QLabel(tr("Ln 1, Col 1"), this);
    statusBar()->addPermanentWidget(m_pythonStatusLabel);
    statusBar()->addPermanentWidget(m_documentStatusLabel);
    statusBar()->addPermanentWidget(m_fileStatusLabel);
    statusBar()->addPermanentWidget(m_cursorStatusLabel);
    statusBar()->addPermanentWidget(new QLabel(tr("Encoding: UTF-8"), this));
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::createScriptActions()
{
    m_newScriptAction = new QAction(this);
    m_openScriptAction = new QAction(this);
    m_saveScriptAction = new QAction(this);
    m_saveAllScriptsAction = new QAction(this);
    m_runScriptAction = new QAction(this);
    m_stopScriptAction = new QAction(this);
    m_closeTabAction = new QAction(this);
    m_closeOtherTabsAction = new QAction(this);
    m_reopenSessionAction = new QAction(this);
    m_settingsAction = new QAction(this);
    m_chooseFileBrowserRootAction = new QAction(this);
    m_commandPaletteAction = new QAction(this);
    m_checkUpdatesAction = new QAction(this);
    m_newScriptAction->setShortcut(QKeySequence::New);
    m_openScriptAction->setShortcut(QKeySequence::Open);
    m_saveScriptAction->setShortcut(QKeySequence::Save);
    m_saveAllScriptsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    m_runScriptAction->setShortcut(Qt::Key_F5);
    m_stopScriptAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F5));
    m_closeTabAction->setShortcut(QKeySequence::Close);
    m_commandPaletteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    m_stopScriptAction->setEnabled(false);

    connect(m_newScriptAction, &QAction::triggered, this, [this] {
        m_workspaceWidget->newDocument();
    });
    connect(m_openScriptAction, &QAction::triggered, this, [this] {
        const QString startDir = m_workspaceManager.fileBrowserRoot();
        const QString filePath = getThemedOpenFileName(
            {
                tr("Open Python Script"),
                startDir,
                tr("Python Files (*.py);;All Files (*)"),
                QFileDialog::ExistingFile,
                QFileDialog::AcceptOpen,
            },
            this);
        if (!filePath.isEmpty() && m_workspaceWidget->openFile(filePath)) {
            m_workspaceManager.addRecentFile(filePath);
        }
    });
    connect(m_saveScriptAction, &QAction::triggered, this, [this] {
        saveCurrentScriptIfNeeded();
    });
    connect(m_saveAllScriptsAction, &QAction::triggered, this, [this] {
        saveAllScripts();
    });
    connect(m_runScriptAction, &QAction::triggered, this, &MainWindow::runCurrentScript);
    connect(m_stopScriptAction, &QAction::triggered, &m_scriptExecutionManager, &core::ScriptExecutionManager::stop);
    connect(m_closeTabAction, &QAction::triggered, this, [this] {
        m_workspaceWidget->closeCurrent();
    });
    connect(m_closeOtherTabsAction, &QAction::triggered, m_workspaceWidget, &EditorWorkspaceWidget::closeOtherEditors);
    connect(m_reopenSessionAction, &QAction::triggered, this, &MainWindow::reopenLastSession);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    connect(m_chooseFileBrowserRootAction, &QAction::triggered, this, &MainWindow::chooseFileBrowserRoot);
    connect(m_commandPaletteAction, &QAction::triggered, this, &MainWindow::openCommandPalette);
    connect(m_checkUpdatesAction, &QAction::triggered, this, &MainWindow::checkForUpdates);
}

void MainWindow::refreshRecentFilesMenu()
{
    if (m_recentFilesMenu == nullptr) {
        return;
    }

    m_recentFilesMenu->clear();
    const QStringList recentFiles = m_workspaceManager.recentFiles();
    if (recentFiles.isEmpty()) {
        QAction *emptyAction = m_recentFilesMenu->addAction(tr("No Recent Files"));
        emptyAction->setEnabled(false);
        return;
    }

    for (const QString &filePath : recentFiles) {
        QAction *action = m_recentFilesMenu->addAction(QFileInfo(filePath).fileName());
        action->setData(filePath);
        action->setToolTip(filePath);
        connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
    }
}

void MainWindow::registerBuiltInCommands()
{
    using pyraqt::core::CommandDescriptor;

    CommandDescriptor runCommand;
    runCommand.id = QStringLiteral("builtin.run_script");
    runCommand.ownerId = QStringLiteral("builtin");
    runCommand.title = tr("Run Script");
    runCommand.description = tr("Run the current script.");
    runCommand.source = tr("Built-in");
    runCommand.keywords = QStringList{QStringLiteral("script"), QStringLiteral("run")};
    runCommand.handler = [this] { runCurrentScript(); };
    m_commandManager.registerCommand(runCommand);

    CommandDescriptor saveCommand;
    saveCommand.id = QStringLiteral("builtin.save_script");
    saveCommand.ownerId = QStringLiteral("builtin");
    saveCommand.title = tr("Save Script");
    saveCommand.description = tr("Save the current script.");
    saveCommand.source = tr("Built-in");
    saveCommand.keywords = QStringList{QStringLiteral("script"), QStringLiteral("save")};
    saveCommand.handler = [this] { saveCurrentScriptIfNeeded(); };
    m_commandManager.registerCommand(saveCommand);

    CommandDescriptor saveAllCommand;
    saveAllCommand.id = QStringLiteral("builtin.save_all_scripts");
    saveAllCommand.ownerId = QStringLiteral("builtin");
    saveAllCommand.title = tr("Save All Scripts");
    saveAllCommand.description = tr("Save every opened script.");
    saveAllCommand.source = tr("Built-in");
    saveAllCommand.keywords = QStringList{QStringLiteral("script"), QStringLiteral("save"), QStringLiteral("all")};
    saveAllCommand.handler = [this] { saveAllScripts(); };
    m_commandManager.registerCommand(saveAllCommand);

    CommandDescriptor paletteCommand;
    paletteCommand.id = QStringLiteral("builtin.open_command_palette");
    paletteCommand.ownerId = QStringLiteral("builtin");
    paletteCommand.title = tr("Open Command Palette");
    paletteCommand.description = tr("Search and execute available commands.");
    paletteCommand.source = tr("Built-in");
    paletteCommand.keywords = QStringList{QStringLiteral("command"), QStringLiteral("palette")};
    paletteCommand.handler = [this] { openCommandPalette(); };
    m_commandManager.registerCommand(paletteCommand);

    CommandDescriptor settingsCommand;
    settingsCommand.id = QStringLiteral("builtin.open_settings");
    settingsCommand.ownerId = QStringLiteral("builtin");
    settingsCommand.title = tr("Open Settings");
    settingsCommand.description = tr("Open the application settings dialog.");
    settingsCommand.source = tr("Built-in");
    settingsCommand.keywords = QStringList{QStringLiteral("settings"), QStringLiteral("preferences")};
    settingsCommand.handler = [this] { openSettings(); };
    m_commandManager.registerCommand(settingsCommand);

    CommandDescriptor reopenCommand;
    reopenCommand.id = QStringLiteral("builtin.reopen_last_session");
    reopenCommand.ownerId = QStringLiteral("builtin");
    reopenCommand.title = tr("Reopen Last Session");
    reopenCommand.description = tr("Restore the previous session files.");
    reopenCommand.source = tr("Built-in");
    reopenCommand.keywords = QStringList{QStringLiteral("session"), QStringLiteral("restore")};
    reopenCommand.handler = [this] { reopenLastSession(); };
    m_commandManager.registerCommand(reopenCommand);

    CommandDescriptor closeTabCommand;
    closeTabCommand.id = QStringLiteral("builtin.close_tab");
    closeTabCommand.ownerId = QStringLiteral("builtin");
    closeTabCommand.title = tr("Close Tab");
    closeTabCommand.description = tr("Close the current editor tab.");
    closeTabCommand.source = tr("Built-in");
    closeTabCommand.keywords = QStringList{QStringLiteral("tab"), QStringLiteral("close")};
    closeTabCommand.handler = [this] { m_workspaceWidget->closeCurrent(); };
    m_commandManager.registerCommand(closeTabCommand);

    CommandDescriptor updateCommand;
    updateCommand.id = QStringLiteral("builtin.check_updates");
    updateCommand.ownerId = QStringLiteral("builtin");
    updateCommand.title = tr("Check for Updates");
    updateCommand.description = tr("Check whether a newer PyraQt build is available.");
    updateCommand.source = tr("Built-in");
    updateCommand.keywords = QStringList{QStringLiteral("update"), QStringLiteral("release")};
    updateCommand.handler = [this] { checkForUpdates(); };
    m_commandManager.registerCommand(updateCommand);
}

void MainWindow::runCurrentScript()
{
    if (!saveCurrentScriptIfNeeded()) {
        return;
    }

    const QString filePath = currentScriptFilePath();
    if (filePath.isEmpty()) {
        appendConsoleLine(QStringLiteral("error"), tr("Cannot run without a saved script file."));
        return;
    }

    if (!m_scriptExecutionManager.runFile(filePath)) {
        appendConsoleLine(QStringLiteral("error"), tr("Failed to start script execution."));
    }
}

void MainWindow::checkForUpdates()
{
    const pyraqt::core::UpdateCheckResult result = m_updateManager.checkForUpdates(true);
    QMessageBox::information(this, tr("Check for Updates"), result.message);
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(
        m_configManager,
        m_themeManager,
        m_i18nManager,
        m_pythonRuntimeManager,
        m_updateManager,
        m_workspaceManager,
        this);
    dialog.exec();
    if (m_fileBrowserPanel != nullptr) {
        m_fileBrowserPanel->setRootPath(m_workspaceManager.fileBrowserRoot());
    }
}

void MainWindow::reopenLastSession()
{
    if (m_workspaceWidget == nullptr) {
        return;
    }
    if (!m_workspaceWidget->closeAllEditors()) {
        return;
    }
    m_workspaceWidget->restoreSession(m_workspaceManager.restoreSession());
}

void MainWindow::chooseFileBrowserRoot()
{
    const QString directory = getThemedExistingDirectory(
        {
            tr("Choose File Browser Root"),
            m_workspaceManager.fileBrowserRoot(),
            QString(),
            QFileDialog::Directory,
            QFileDialog::AcceptOpen,
        },
        this);
    if (directory.isEmpty()) {
        return;
    }
    m_workspaceManager.setFileBrowserRoot(directory);
    const bool saved = m_configManager.save();
    if (!saved) {
        m_logManager.warning(tr("Failed to save file browser root."));
    }
    if (m_fileBrowserPanel != nullptr) {
        m_fileBrowserPanel->setRootPath(directory);
    }
}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action == nullptr) {
        return;
    }
    const QString filePath = action->data().toString();
    if (!filePath.isEmpty() && m_workspaceWidget->openFile(filePath)) {
        m_workspaceManager.addRecentFile(filePath);
    }
}

bool MainWindow::saveAllScripts()
{
    for (int index = 0; index < m_workspaceWidget->editorCount(); ++index) {
        auto *editor = m_workspaceWidget->editorAt(index);
        if (editor == nullptr || !editor->isModified()) {
            continue;
        }
        if (editor->currentFilePath().isEmpty()) {
            return false;
        }
    }
    return m_workspaceWidget->saveAll();
}

void MainWindow::showRecoveryPromptIfNeeded()
{
    if (!m_crashRecoveryManager.didPreviousRunCrash()) {
        return;
    }

    QMessageBox::warning(
        this,
        tr("Session Recovery"),
        tr("%1\nCrash log: %2").arg(m_crashRecoveryManager.recoveryMessage(), m_crashRecoveryManager.crashLogPath()));
}

bool MainWindow::saveCurrentScriptIfNeeded()
{
    if (m_workspaceWidget == nullptr || !m_workspaceWidget->hasAvailableEditor()) {
        return false;
    }

    ScriptEditorWidget *editor = m_workspaceWidget->currentEditor();
    if (editor == nullptr) {
        return false;
    }

    if (!editor->isModified() && !editor->currentFilePath().isEmpty()) {
        return true;
    }

    QString filePath = editor->currentFilePath();
    if (filePath.isEmpty()) {
        const QString startDir = m_workspaceManager.fileBrowserRoot();
        filePath = getThemedSaveFileName(
            {
                tr("Save Python Script"),
                startDir,
                tr("Python Files (*.py);;All Files (*)"),
                QFileDialog::AnyFile,
                QFileDialog::AcceptSave,
            },
            this);
        if (filePath.isEmpty()) {
            return false;
        }
        const bool saved = m_workspaceWidget->saveCurrentAs(filePath);
        if (saved) {
            m_workspaceManager.addRecentFile(filePath);
        }
        return saved;
    }

    const bool saved = m_workspaceWidget->saveCurrent();
    if (saved) {
        m_workspaceManager.addRecentFile(filePath);
    }
    return saved;
}

QString MainWindow::currentScriptFilePath() const
{
    return m_workspaceWidget != nullptr ? m_workspaceWidget->currentFilePath() : QString();
}

void MainWindow::openCommandPalette()
{
    CommandPaletteDialog dialog(m_commandManager, this);
    dialog.exec();
}

void MainWindow::appendConsoleLine(const QString &prefix, const QString &message)
{
    if (m_console == nullptr || message.isEmpty()) {
        return;
    }
    m_console->appendPlainText(QStringLiteral("[%1] %2").arg(prefix, message));
}

void MainWindow::restoreSession()
{
    QSettings settings(QStringLiteral("PyraQt"), QStringLiteral("PyraQt"));
    const QByteArray geometry = settings.value(QStringLiteral("mainWindow/geometry")).toByteArray();
    const QByteArray state = settings.value(QStringLiteral("mainWindow/state")).toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    if (!state.isEmpty()) {
        restoreState(state);
    }

    const core::WorkspaceSession session = m_workspaceManager.restoreSession();
    if (m_fileBrowserPanel != nullptr) {
        m_fileBrowserPanel->setRootPath(session.fileBrowserRoot);
    }

    if (m_workspaceManager.restoreLastSessionEnabled()) {
        m_workspaceWidget->restoreSession(session);
    } else {
        m_workspaceWidget->newDocument();
    }
}

void MainWindow::retranslateUi()
{
    const QString filePath = currentScriptFilePath();
    setWindowTitle(filePath.isEmpty() ? tr("PyraQt - Untitled Script") : tr("PyraQt - %1").arg(filePath));

    if (auto *fileMenu = findChild<QMenu *>(QStringLiteral("fileMenu"))) {
        fileMenu->setTitle(tr("&File"));
    }
    if (auto *recentMenu = findChild<QMenu *>(QStringLiteral("recentFilesMenu"))) {
        recentMenu->setTitle(tr("Recent Files"));
    }
    if (auto *viewMenu = findChild<QMenu *>(QStringLiteral("viewMenu"))) {
        viewMenu->setTitle(tr("&View"));
    }
    if (auto *toolsMenu = findChild<QMenu *>(QStringLiteral("toolsMenu"))) {
        toolsMenu->setTitle(tr("&Tools"));
    }
    if (auto *themeMenu = findChild<QMenu *>(QStringLiteral("themeMenu"))) {
        themeMenu->setTitle(tr("&Theme"));
    }
    if (auto *languageMenu = findChild<QMenu *>(QStringLiteral("languageMenu"))) {
        languageMenu->setTitle(tr("&Language"));
    }
    if (auto *quitAction = findChild<QAction *>(QStringLiteral("quitAction"))) {
        quitAction->setText(tr("&Quit"));
    }

    if (m_lightThemeAction != nullptr) {
        m_lightThemeAction->setText(tr("Light"));
    }
    if (m_darkThemeAction != nullptr) {
        m_darkThemeAction->setText(tr("Dark"));
    }
    if (m_englishAction != nullptr) {
        m_englishAction->setText(tr("English"));
    }
    if (m_chineseAction != nullptr) {
        m_chineseAction->setText(tr("Simplified Chinese"));
    }
    if (m_newScriptAction != nullptr) {
        m_newScriptAction->setText(tr("New Script"));
    }
    if (m_openScriptAction != nullptr) {
        m_openScriptAction->setText(tr("Open Script"));
    }
    if (m_saveScriptAction != nullptr) {
        m_saveScriptAction->setText(tr("Save Script"));
    }
    if (m_saveAllScriptsAction != nullptr) {
        m_saveAllScriptsAction->setText(tr("Save All"));
    }
    if (m_runScriptAction != nullptr) {
        m_runScriptAction->setText(tr("Run Script"));
    }
    if (m_stopScriptAction != nullptr) {
        m_stopScriptAction->setText(tr("Stop Script"));
    }
    if (m_closeTabAction != nullptr) {
        m_closeTabAction->setText(tr("Close Tab"));
    }
    if (m_closeOtherTabsAction != nullptr) {
        m_closeOtherTabsAction->setText(tr("Close Other Tabs"));
    }
    if (m_reopenSessionAction != nullptr) {
        m_reopenSessionAction->setText(tr("Reopen Last Session"));
    }
    if (m_settingsAction != nullptr) {
        m_settingsAction->setText(tr("Settings"));
    }
    if (m_chooseFileBrowserRootAction != nullptr) {
        m_chooseFileBrowserRootAction->setText(tr("Set File Browser Root"));
    }
    if (m_commandPaletteAction != nullptr) {
        m_commandPaletteAction->setText(tr("Command Palette"));
    }
    if (m_checkUpdatesAction != nullptr) {
        m_checkUpdatesAction->setText(tr("Check for Updates"));
    }
    if (m_pythonStatusLabel != nullptr) {
        m_pythonStatusLabel->setText(tr("Python: %1").arg(m_pythonRuntimeManager.pythonVersion()));
    }
    if (m_documentStatusLabel != nullptr && m_workspaceWidget != nullptr) {
        ScriptEditorWidget *editor = m_workspaceWidget->currentEditor();
        m_documentStatusLabel->setText(editor != nullptr && editor->isModified() ? tr("Document: Modified") : tr("Document: Saved"));
    }
    if (m_fileStatusLabel != nullptr) {
        m_fileStatusLabel->setText(filePath.isEmpty() ? tr("File: Untitled") : tr("File: %1").arg(QFileInfo(filePath).fileName()));
    }

    statusBar()->showMessage(tr("Ready"));
}

QDockWidget *MainWindow::createTextDock(const QString &objectName, const QString &title, const QString &body)
{
    auto *dock = new QDockWidget(title, this);
    dock->setObjectName(objectName);
    auto *content = new QPlainTextEdit(dock);
    content->setReadOnly(true);
    content->setPlainText(body);
    dock->setWidget(content);
    return dock;
}

} // namespace pyraqt::ui

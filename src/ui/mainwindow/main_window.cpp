#include "ui/mainwindow/main_window.h"

#include "core/command/command_descriptor.h"
#include "core/command/command_manager.h"
#include "core/config/config_manager.h"
#include "core/i18n/i18n_manager.h"
#include "core/logging/log_manager.h"
#include "core/modeling/model_import_manager.h"
#include "core/modeling/model_property_service.h"
#include "core/modeling/model_types.h"
#include "core/plugin/plugin_manager.h"
#include "core/runtime/crash_recovery_manager.h"
#include "core/scripting/python_runtime_manager.h"
#include "core/scripting/script_execution_manager.h"
#include "core/theme/theme_manager.h"
#include "core/update/update_manager.h"
#include "core/update/update_types.h"
#include "core/workspace/workspace_manager.h"
#include "ui/dialogs/command_palette_dialog.h"
#include "ui/dialogs/python_tools_dialog.h"
#include "ui/dialogs/settings_dialog.h"
#include "ui/common/file_dialog_utils.h"
#include "ui/common/icon_utils.h"
#include "ui/editor/editor_placeholder_widget.h"
#include "ui/editor/editor_workspace_widget.h"
#include "ui/editor/model_document_widget.h"
#include "ui/editor/script_editor_widget.h"
#include "ui/panels/files/file_browser_panel.h"
#include "ui/panels/properties/model_properties_panel.h"
#include "ui/panels/plugins/plugin_manager_panel.h"
#include "ui/panels/python/python_console_widget.h"

#include <QAction>
#include <QCloseEvent>
#include <QDockWidget>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
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
    core::ModelImportManager &modelImportManager,
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
    , m_modelImportManager(modelImportManager)
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
    connect(&m_themeManager, &core::ThemeManager::themeChanged, this, [this] {
        applyActionIcons();
        configureToolbar();
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
    if (m_sessionSaved) {
        return;
    }

    QSettings settings(QStringLiteral("PyraQt"), QStringLiteral("PyraQt"));
    settings.setValue(QStringLiteral("mainWindow/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("mainWindow/state"), saveState());

    if (m_workspaceWidget != nullptr) {
        m_workspaceManager.saveSession(
            m_workspaceWidget->captureSession(m_workspaceManager.recentFiles(),
                m_fileBrowserPanel != nullptr ? m_fileBrowserPanel->rootPath() : m_workspaceManager.fileBrowserRoot()));
    }

    m_sessionSaved = true;
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSession();
    QMainWindow::closeEvent(event);
}

void MainWindow::createCentralEditor()
{
    m_workspaceWidget = new EditorWorkspaceWidget(m_themeManager, m_modelImportManager, &m_pythonRuntimeManager, this);
    m_workspaceWidget->setObjectName(QStringLiteral("editorWorkspace"));
    setCentralWidget(m_workspaceWidget);

    connect(m_workspaceWidget, &EditorWorkspaceWidget::documentModificationChanged, this, [this](bool modified) {
        if (m_documentStatusLabel != nullptr) {
            m_documentStatusLabel->setText(modified ? tr("Document: Modified") : tr("Document: Saved"));
        }
        updateTabActionStates();
    });
    connect(m_workspaceWidget, &EditorWorkspaceWidget::openFilesChanged, this, [this](const QStringList &) {
        updateTabActionStates();
        updatePropertiesDockVisibility();
    });
    connect(m_workspaceWidget, &EditorWorkspaceWidget::editorAvailabilityChanged, this, [this](bool) {
        updateTabActionStates();
    });
    connect(m_workspaceWidget, &EditorWorkspaceWidget::currentFilePathChanged, this, [this](const QString &filePath) {
        if (m_fileStatusLabel != nullptr) {
            m_fileStatusLabel->setText(filePath.isEmpty() ? tr("File: Untitled") : tr("File: %1").arg(QFileInfo(filePath).fileName()));
        }
        if (filePath.isEmpty()) {
            setWindowTitle(tr("PyraQt - Untitled"));
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
        updateTabActionStates();
        updatePropertiesDockVisibility();
    });
    connect(m_workspaceWidget, &EditorWorkspaceWidget::currentCursorChanged, this, [this](int line, int column) {
        if (m_cursorStatusLabel != nullptr) {
            if (line < 0 || column < 0) {
                m_cursorStatusLabel->setText(tr("Preview Unavailable"));
            } else if (line <= 0 || column <= 0) {
                m_cursorStatusLabel->setText(tr("Model View"));
            } else {
                m_cursorStatusLabel->setText(tr("Ln %1, Col %2").arg(line).arg(column));
            }
        }
    });
    connect(m_workspaceWidget, &EditorWorkspaceWidget::modelDocumentChanged, this, [this](const core::ModelDocument &document) {
        if (m_documentStatusLabel == nullptr) {
            return;
        }
        if (document.filePath.isEmpty()) {
            if (m_workspaceWidget != nullptr
                && m_workspaceWidget->currentDocumentKind() == EditorWorkspaceWidget::DocumentKind::PreviewUnavailable) {
                m_documentStatusLabel->setText(tr("Document: Preview Unavailable"));
                updateModelActionStates();
                return;
            }
            if (m_modelPropertiesPanel != nullptr) {
                m_modelPropertiesPanel->showPlaceholder(tr("Select a model to inspect its properties."));
            }
            updateModelActionStates();
            return;
        }
        m_documentStatusLabel->setText(document.isValid ? tr("Document: Model Loaded") : tr("Document: Load Failed"));
        if (m_modelPropertiesPanel != nullptr) {
            m_modelPropertiesPanel->setModelDocument(document);
            m_modelPropertiesPanel->clearSelection();
        }
        if (m_cursorStatusLabel != nullptr) {
            m_cursorStatusLabel->setText(
                tr("%1 / %2")
                    .arg(core::ModelPropertyService::formatDisplayMode(document.displayMode))
                    .arg(core::ModelPropertyService::formatSelectionMode(document.selectionMode)));
        }
        updateModelActionStates();
        updatePropertiesDockVisibility();
    });
    connect(m_workspaceWidget, &EditorWorkspaceWidget::modelSelectionChanged, this, [this](const core::ModelSelectionInfo &selection) {
        if (m_modelPropertiesPanel == nullptr) {
            return;
        }
        if (!selection.hasSelection) {
            m_modelPropertiesPanel->clearSelection();
        } else {
            m_modelPropertiesPanel->setSelectionInfo(selection);
        }
        if (m_cursorStatusLabel != nullptr && selection.kind != core::ModelSelectionKind::None) {
            m_cursorStatusLabel->setText(core::ModelPropertyService::formatSelectionType(selection.kind));
        }
    });
    connect(m_workspaceWidget, &EditorWorkspaceWidget::requestCloseConfirmation, this,
        [this](const QString &title, const QString &message, bool *accepted) {
            if (accepted == nullptr) {
                return;
            }
            *accepted = QMessageBox::question(this, title, message, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
        });
    connect(m_workspaceWidget, &EditorWorkspaceWidget::openPathFailed, this, [this](const QString &, const QString &message) {
        if (!message.isEmpty()) {
            QMessageBox::warning(this, tr("Open File"), message);
        }
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
        openPath(path);
    });
    connect(m_fileBrowserPanel, &FileBrowserPanel::renameRequested, this, &MainWindow::renamePathFromFileBrowser);
    connect(m_fileBrowserPanel, &FileBrowserPanel::deleteRequested, this, &MainWindow::deletePathFromFileBrowser);

    auto *pluginDock = new QDockWidget(tr("Plugins"), this);
    pluginDock->setObjectName(QStringLiteral("pluginDock"));
    m_pluginManagerPanel = new PluginManagerPanel(m_pluginManager, pluginDock);
    pluginDock->setWidget(m_pluginManagerPanel);
    addDockWidget(Qt::LeftDockWidgetArea, pluginDock);

    m_propertiesDock = new QDockWidget(tr("Properties"), this);
    m_propertiesDock->setObjectName(QStringLiteral("propertiesDock"));
    m_propertiesDock->setMinimumWidth(280);
    m_modelPropertiesPanel = new ModelPropertiesPanel(m_propertiesDock);
    m_propertiesDock->setWidget(m_modelPropertiesPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_propertiesDock);
    updatePropertiesDockVisibility();

    auto *consoleDock = new QDockWidget(tr("Python Console"), this);
    consoleDock->setObjectName(QStringLiteral("consoleDock"));
    m_console = new PythonConsoleWidget(m_pythonRuntimeManager, m_scriptExecutionManager, &m_themeManager, consoleDock);
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
    fileMenu->addAction(m_saveScriptAsAction);
    fileMenu->addAction(m_saveAllScriptsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_closeTabAction);
    fileMenu->addAction(m_closeOtherTabsAction);
    fileMenu->addAction(m_closeRightTabsAction);
    fileMenu->addAction(m_closeAllTabsAction);
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
    toolsMenu->addAction(m_pythonToolsAction);
    toolsMenu->addAction(m_settingsAction);
    toolsMenu->addAction(m_chooseFileBrowserRootAction);
    toolsMenu->addAction(m_checkUpdatesAction);

    auto *themeMenu = viewMenu->addMenu(QString());
    themeMenu->setObjectName(QStringLiteral("themeMenu"));
    auto *modelViewMenu = viewMenu->addMenu(QString());
    modelViewMenu->setObjectName(QStringLiteral("modelViewMenu"));
    auto *displayModeMenu = modelViewMenu->addMenu(QString());
    displayModeMenu->setObjectName(QStringLiteral("displayModeMenu"));
    auto *selectionMenu = modelViewMenu->addMenu(QString());
    selectionMenu->setObjectName(QStringLiteral("selectionMenu"));

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

    modelViewMenu->addAction(m_fitAllAction);
    modelViewMenu->addAction(m_viewFrontAction);
    modelViewMenu->addAction(m_viewBackAction);
    modelViewMenu->addAction(m_viewLeftAction);
    modelViewMenu->addAction(m_viewRightAction);
    modelViewMenu->addAction(m_viewTopAction);
    modelViewMenu->addAction(m_viewBottomAction);
    modelViewMenu->addAction(m_viewIsoAction);
    displayModeMenu->addAction(m_wireframeAction);
    displayModeMenu->addAction(m_shadedAction);
    displayModeMenu->addAction(m_shadedEdgesAction);
    selectionMenu->addAction(m_selectShapeAction);
    selectionMenu->addAction(m_selectFaceAction);
    selectionMenu->addAction(m_selectEdgeAction);
    selectionMenu->addAction(m_selectVertexAction);
    selectionMenu->addAction(m_clearSelectionAction);

    m_fileToolBar = createGroupedToolBar(QStringLiteral("fileToolBar"),
        {m_newScriptAction, m_openScriptAction, m_saveScriptAction, m_saveScriptAsAction, m_saveAllScriptsAction},
        tr("File Tools"));
    m_scriptToolBar = createGroupedToolBar(
        QStringLiteral("scriptToolBar"), {m_runScriptAction, m_stopScriptAction, m_commandPaletteAction}, tr("Script Tools"));
    m_workspaceToolBar = createGroupedToolBar(
        QStringLiteral("workspaceToolBar"), {m_settingsAction, m_checkUpdatesAction, m_chooseFileBrowserRootAction}, tr("Workspace Tools"));
    m_viewToolBar = createGroupedToolBar(
        QStringLiteral("viewToolBar"), {m_fitAllAction, m_wireframeAction, m_shadedAction, m_shadedEdgesAction}, tr("View Tools"));
    m_selectionToolBar = createGroupedToolBar(
        QStringLiteral("selectionToolBar"), {m_selectFaceAction, m_selectEdgeAction, m_clearSelectionAction}, tr("Selection Tools"));
    m_themeToolBar = createGroupedToolBar(
        QStringLiteral("themeToolBar"), {m_lightThemeAction, m_darkThemeAction}, tr("Theme Tools"));
    applyActionIcons();
    configureToolbar();
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
    m_saveScriptAsAction = new QAction(this);
    m_saveAllScriptsAction = new QAction(this);
    m_runScriptAction = new QAction(this);
    m_stopScriptAction = new QAction(this);
    m_closeTabAction = new QAction(this);
    m_closeOtherTabsAction = new QAction(this);
    m_closeRightTabsAction = new QAction(this);
    m_closeAllTabsAction = new QAction(this);
    m_reopenSessionAction = new QAction(this);
    m_settingsAction = new QAction(this);
    m_pythonToolsAction = new QAction(this);
    m_chooseFileBrowserRootAction = new QAction(this);
    m_commandPaletteAction = new QAction(this);
    m_checkUpdatesAction = new QAction(this);
    m_fitAllAction = new QAction(this);
    m_viewFrontAction = new QAction(this);
    m_viewBackAction = new QAction(this);
    m_viewLeftAction = new QAction(this);
    m_viewRightAction = new QAction(this);
    m_viewTopAction = new QAction(this);
    m_viewBottomAction = new QAction(this);
    m_viewIsoAction = new QAction(this);
    m_wireframeAction = new QAction(this);
    m_shadedAction = new QAction(this);
    m_shadedEdgesAction = new QAction(this);
    m_selectShapeAction = new QAction(this);
    m_selectFaceAction = new QAction(this);
    m_selectEdgeAction = new QAction(this);
    m_selectVertexAction = new QAction(this);
    m_clearSelectionAction = new QAction(this);
    m_newScriptAction->setShortcut(QKeySequence::New);
    m_openScriptAction->setShortcut(QKeySequence::Open);
    m_saveScriptAction->setShortcut(QKeySequence::Save);
    m_saveScriptAsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    m_saveAllScriptsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S));
    m_runScriptAction->setShortcut(Qt::Key_F5);
    m_stopScriptAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F5));
    m_closeTabAction->setShortcut(QKeySequence::Close);
    m_commandPaletteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    m_fitAllAction->setShortcut(QKeySequence(Qt::Key_F));
    m_wireframeAction->setCheckable(true);
    m_shadedAction->setCheckable(true);
    m_shadedEdgesAction->setCheckable(true);
    m_selectShapeAction->setCheckable(true);
    m_selectFaceAction->setCheckable(true);
    m_selectEdgeAction->setCheckable(true);
    m_selectVertexAction->setCheckable(true);
    m_stopScriptAction->setEnabled(false);
    updateTabActionStates();
    updateModelActionStates();

    connect(m_newScriptAction, &QAction::triggered, this, [this] {
        m_workspaceWidget->newDocument();
    });
    connect(m_openScriptAction, &QAction::triggered, this, [this] {
        const QString startDir = m_workspaceManager.fileBrowserRoot();
        const QString filePath = getThemedOpenFileName(
            {
                tr("Open File"),
                startDir,
                tr("Editable Text and Models (*.txt *.md *.json *.xml *.yaml *.yml *.toml *.ini *.cfg *.conf *.log *.csv *.py *.cpp *.c *.h *.hpp *.qml *.js *.ts *.html *.css *.stp *.step *.brep);;All Files (*)"),
                QString(),
                QFileDialog::ExistingFile,
                QFileDialog::AcceptOpen,
            },
            this);
        if (!filePath.isEmpty()) {
            openPath(filePath);
        }
    });
    connect(m_saveScriptAction, &QAction::triggered, this, [this] {
        saveCurrentScriptIfNeeded();
    });
    connect(m_saveScriptAsAction, &QAction::triggered, this, [this] {
        saveCurrentScriptAs();
    });
    connect(m_saveAllScriptsAction, &QAction::triggered, this, [this] {
        saveAllScripts();
    });
    connect(m_runScriptAction, &QAction::triggered, this, &MainWindow::runCurrentScript);
    connect(m_stopScriptAction, &QAction::triggered, &m_scriptExecutionManager, &core::ScriptExecutionManager::stop);
    connect(m_closeTabAction, &QAction::triggered, this, [this] {
        m_workspaceWidget->closeCurrent();
    });
    connect(m_closeOtherTabsAction, &QAction::triggered, this, [this] {
        m_workspaceWidget->closeOtherEditors();
    });
    connect(m_closeRightTabsAction, &QAction::triggered, this, [this] {
        m_workspaceWidget->closeEditorsToRight();
    });
    connect(m_closeAllTabsAction, &QAction::triggered, this, [this] {
        m_workspaceWidget->closeAllEditors();
    });
    connect(m_reopenSessionAction, &QAction::triggered, this, &MainWindow::reopenLastSession);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    connect(m_pythonToolsAction, &QAction::triggered, this, &MainWindow::openPythonTools);
    connect(m_chooseFileBrowserRootAction, &QAction::triggered, this, &MainWindow::chooseFileBrowserRoot);
    connect(m_commandPaletteAction, &QAction::triggered, this, &MainWindow::openCommandPalette);
    connect(m_checkUpdatesAction, &QAction::triggered, this, &MainWindow::checkForUpdates);
    connect(m_fitAllAction, &QAction::triggered, this, &MainWindow::fitModelView);
    connect(m_viewFrontAction, &QAction::triggered, this, &MainWindow::setModelViewPreset);
    connect(m_viewBackAction, &QAction::triggered, this, &MainWindow::setModelViewPreset);
    connect(m_viewLeftAction, &QAction::triggered, this, &MainWindow::setModelViewPreset);
    connect(m_viewRightAction, &QAction::triggered, this, &MainWindow::setModelViewPreset);
    connect(m_viewTopAction, &QAction::triggered, this, &MainWindow::setModelViewPreset);
    connect(m_viewBottomAction, &QAction::triggered, this, &MainWindow::setModelViewPreset);
    connect(m_viewIsoAction, &QAction::triggered, this, &MainWindow::setModelViewPreset);
    connect(m_wireframeAction, &QAction::triggered, this, &MainWindow::setModelDisplayMode);
    connect(m_shadedAction, &QAction::triggered, this, &MainWindow::setModelDisplayMode);
    connect(m_shadedEdgesAction, &QAction::triggered, this, &MainWindow::setModelDisplayMode);
    connect(m_selectShapeAction, &QAction::triggered, this, &MainWindow::setModelSelectionMode);
    connect(m_selectFaceAction, &QAction::triggered, this, &MainWindow::setModelSelectionMode);
    connect(m_selectEdgeAction, &QAction::triggered, this, &MainWindow::setModelSelectionMode);
    connect(m_selectVertexAction, &QAction::triggered, this, &MainWindow::setModelSelectionMode);
    connect(m_clearSelectionAction, &QAction::triggered, this, &MainWindow::clearModelSelection);
    configureActionPresentation();
    applyActionIcons();
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
    runCommand.description = tr("Run the current Python file.");
    runCommand.source = tr("Built-in");
    runCommand.keywords = QStringList{QStringLiteral("script"), QStringLiteral("run")};
    runCommand.handler = [this] { runCurrentScript(); };
    m_commandManager.registerCommand(runCommand);

    CommandDescriptor saveCommand;
    saveCommand.id = QStringLiteral("builtin.save_script");
    saveCommand.ownerId = QStringLiteral("builtin");
    saveCommand.title = tr("Save File");
    saveCommand.description = tr("Save the current file.");
    saveCommand.source = tr("Built-in");
    saveCommand.keywords = QStringList{QStringLiteral("script"), QStringLiteral("save")};
    saveCommand.handler = [this] { saveCurrentScriptIfNeeded(); };
    m_commandManager.registerCommand(saveCommand);

    CommandDescriptor saveAllCommand;
    saveAllCommand.id = QStringLiteral("builtin.save_all_scripts");
    saveAllCommand.ownerId = QStringLiteral("builtin");
    saveAllCommand.title = tr("Save All Files");
    saveAllCommand.description = tr("Save every opened editable file.");
    saveAllCommand.source = tr("Built-in");
    saveAllCommand.keywords = QStringList{QStringLiteral("script"), QStringLiteral("save"), QStringLiteral("all")};
    saveAllCommand.handler = [this] { saveAllScripts(); };
    m_commandManager.registerCommand(saveAllCommand);

    CommandDescriptor saveAsCommand;
    saveAsCommand.id = QStringLiteral("builtin.save_script_as");
    saveAsCommand.ownerId = QStringLiteral("builtin");
    saveAsCommand.title = tr("Save File As");
    saveAsCommand.description = tr("Save the current file to a new path.");
    saveAsCommand.source = tr("Built-in");
    saveAsCommand.keywords = QStringList{QStringLiteral("script"), QStringLiteral("save"), QStringLiteral("as")};
    saveAsCommand.handler = [this] { saveCurrentScriptAs(); };
    m_commandManager.registerCommand(saveAsCommand);

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

    CommandDescriptor closeOtherTabsCommand;
    closeOtherTabsCommand.id = QStringLiteral("builtin.close_other_tabs");
    closeOtherTabsCommand.ownerId = QStringLiteral("builtin");
    closeOtherTabsCommand.title = tr("Close Other Tabs");
    closeOtherTabsCommand.description = tr("Close all tabs except the current one.");
    closeOtherTabsCommand.source = tr("Built-in");
    closeOtherTabsCommand.keywords = QStringList{QStringLiteral("tab"), QStringLiteral("close"), QStringLiteral("other")};
    closeOtherTabsCommand.handler = [this] { m_workspaceWidget->closeOtherEditors(); };
    m_commandManager.registerCommand(closeOtherTabsCommand);

    CommandDescriptor closeTabsToRightCommand;
    closeTabsToRightCommand.id = QStringLiteral("builtin.close_tabs_to_right");
    closeTabsToRightCommand.ownerId = QStringLiteral("builtin");
    closeTabsToRightCommand.title = tr("Close Tabs to the Right");
    closeTabsToRightCommand.description = tr("Close every tab to the right of the current tab.");
    closeTabsToRightCommand.source = tr("Built-in");
    closeTabsToRightCommand.keywords = QStringList{QStringLiteral("tab"), QStringLiteral("close"), QStringLiteral("right")};
    closeTabsToRightCommand.handler = [this] { m_workspaceWidget->closeEditorsToRight(); };
    m_commandManager.registerCommand(closeTabsToRightCommand);

    CommandDescriptor closeAllTabsCommand;
    closeAllTabsCommand.id = QStringLiteral("builtin.close_all_tabs");
    closeAllTabsCommand.ownerId = QStringLiteral("builtin");
    closeAllTabsCommand.title = tr("Close All Tabs");
    closeAllTabsCommand.description = tr("Close every open editor tab.");
    closeAllTabsCommand.source = tr("Built-in");
    closeAllTabsCommand.keywords = QStringList{QStringLiteral("tab"), QStringLiteral("close"), QStringLiteral("all")};
    closeAllTabsCommand.handler = [this] { m_workspaceWidget->closeAllEditors(); };
    m_commandManager.registerCommand(closeAllTabsCommand);

    CommandDescriptor openFileCommand;
    openFileCommand.id = QStringLiteral("builtin.open_file");
    openFileCommand.ownerId = QStringLiteral("builtin");
    openFileCommand.title = tr("Open File");
    openFileCommand.description = tr("Open a Python script or supported model file.");
    openFileCommand.source = tr("Built-in");
    openFileCommand.keywords = QStringList{QStringLiteral("open"), QStringLiteral("file"), QStringLiteral("model"), QStringLiteral("script")};
    openFileCommand.handler = [this] { m_openScriptAction->trigger(); };
    m_commandManager.registerCommand(openFileCommand);

    CommandDescriptor updateCommand;
    updateCommand.id = QStringLiteral("builtin.check_updates");
    updateCommand.ownerId = QStringLiteral("builtin");
    updateCommand.title = tr("Check for Updates");
    updateCommand.description = tr("Check whether a newer PyraQt build is available.");
    updateCommand.source = tr("Built-in");
    updateCommand.keywords = QStringList{QStringLiteral("update"), QStringLiteral("release")};
    updateCommand.handler = [this] { checkForUpdates(); };
    m_commandManager.registerCommand(updateCommand);

    CommandDescriptor fitAllCommand;
    fitAllCommand.id = QStringLiteral("builtin.model_fit_all");
    fitAllCommand.ownerId = QStringLiteral("builtin");
    fitAllCommand.title = tr("Model: Fit All");
    fitAllCommand.description = tr("Fit the current model in view.");
    fitAllCommand.source = tr("Built-in");
    fitAllCommand.keywords = QStringList{QStringLiteral("model"), QStringLiteral("fit"), QStringLiteral("view")};
    fitAllCommand.handler = [this] { fitModelView(); };
    m_commandManager.registerCommand(fitAllCommand);

    const auto registerModelCommand = [this](const QString &id, const QString &title, const QString &description,
                                         const QStringList &keywords, std::function<void()> handler) {
        CommandDescriptor command;
        command.id = id;
        command.ownerId = QStringLiteral("builtin");
        command.title = title;
        command.description = description;
        command.source = tr("Built-in");
        command.keywords = keywords;
        command.handler = std::move(handler);
        m_commandManager.registerCommand(command);
    };

    registerModelCommand(QStringLiteral("builtin.model_view_front"), tr("Model: View Front"), tr("Switch to front view."),
        {QStringLiteral("model"), QStringLiteral("front"), QStringLiteral("view")},
        [this] { m_viewFrontAction->trigger(); });
    registerModelCommand(QStringLiteral("builtin.model_view_back"), tr("Model: View Back"), tr("Switch to back view."),
        {QStringLiteral("model"), QStringLiteral("back"), QStringLiteral("view")},
        [this] { m_viewBackAction->trigger(); });
    registerModelCommand(QStringLiteral("builtin.model_view_left"), tr("Model: View Left"), tr("Switch to left view."),
        {QStringLiteral("model"), QStringLiteral("left"), QStringLiteral("view")},
        [this] { m_viewLeftAction->trigger(); });
    registerModelCommand(QStringLiteral("builtin.model_view_right"), tr("Model: View Right"), tr("Switch to right view."),
        {QStringLiteral("model"), QStringLiteral("right"), QStringLiteral("view")},
        [this] { m_viewRightAction->trigger(); });
    registerModelCommand(QStringLiteral("builtin.model_view_top"), tr("Model: View Top"), tr("Switch to top view."),
        {QStringLiteral("model"), QStringLiteral("top"), QStringLiteral("view")},
        [this] { m_viewTopAction->trigger(); });
    registerModelCommand(QStringLiteral("builtin.model_view_bottom"), tr("Model: View Bottom"), tr("Switch to bottom view."),
        {QStringLiteral("model"), QStringLiteral("bottom"), QStringLiteral("view")},
        [this] { m_viewBottomAction->trigger(); });
    registerModelCommand(QStringLiteral("builtin.model_view_isometric"), tr("Model: View Isometric"), tr("Switch to isometric view."),
        {QStringLiteral("model"), QStringLiteral("isometric"), QStringLiteral("view"), QStringLiteral("iso")},
        [this] { m_viewIsoAction->trigger(); });
    registerModelCommand(QStringLiteral("builtin.model_display_wireframe"), tr("Model: Wireframe"), tr("Display model in wireframe mode."),
        {QStringLiteral("model"), QStringLiteral("wireframe"), QStringLiteral("display")},
        [this] { m_wireframeAction->trigger(); });
    registerModelCommand(QStringLiteral("builtin.model_display_shaded"), tr("Model: Shaded"), tr("Display model in shaded mode."),
        {QStringLiteral("model"), QStringLiteral("shaded"), QStringLiteral("display")},
        [this] { m_shadedAction->trigger(); });
    registerModelCommand(QStringLiteral("builtin.model_display_shaded_edges"), tr("Model: Shaded With Edges"),
        tr("Display model in shaded mode with edges."), {QStringLiteral("model"), QStringLiteral("shaded"), QStringLiteral("edges"), QStringLiteral("display")},
        [this] { m_shadedEdgesAction->trigger(); });
    registerModelCommand(QStringLiteral("builtin.model_select_shape"), tr("Model: Select Shape"), tr("Enable whole-shape selection."),
        {QStringLiteral("model"), QStringLiteral("select"), QStringLiteral("shape")},
        [this] { m_selectShapeAction->trigger(); });
    registerModelCommand(QStringLiteral("builtin.model_select_face"), tr("Model: Select Face"), tr("Enable face selection."),
        {QStringLiteral("model"), QStringLiteral("select"), QStringLiteral("face")},
        [this] { m_selectFaceAction->trigger(); });
    registerModelCommand(QStringLiteral("builtin.model_select_edge"), tr("Model: Select Edge"), tr("Enable edge selection."),
        {QStringLiteral("model"), QStringLiteral("select"), QStringLiteral("edge")},
        [this] { m_selectEdgeAction->trigger(); });
    registerModelCommand(QStringLiteral("builtin.model_select_vertex"), tr("Model: Select Vertex"), tr("Enable vertex selection."),
        {QStringLiteral("model"), QStringLiteral("select"), QStringLiteral("vertex"), QStringLiteral("point")},
        [this] { m_selectVertexAction->trigger(); });
    registerModelCommand(QStringLiteral("builtin.model_clear_selection"), tr("Model: Clear Selection"), tr("Clear the current model selection."),
        {QStringLiteral("model"), QStringLiteral("clear"), QStringLiteral("selection")},
        [this] { m_clearSelectionAction->trigger(); });
}

void MainWindow::runCurrentScript()
{
    if (m_workspaceWidget == nullptr || !m_workspaceWidget->currentFileRunnable()) {
        return;
    }

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

void MainWindow::fitModelView()
{
    if (auto *documentWidget = m_workspaceWidget->currentModelDocumentWidget()) {
        documentWidget->fitAll();
    }
}

void MainWindow::setModelViewPreset()
{
    if (auto *documentWidget = m_workspaceWidget->currentModelDocumentWidget()) {
        QAction *action = qobject_cast<QAction *>(sender());
        if (action != nullptr) {
            documentWidget->setStandardView(action->text());
        }
    }
}

void MainWindow::setModelDisplayMode()
{
    if (auto *documentWidget = m_workspaceWidget->currentModelDocumentWidget()) {
        QAction *action = qobject_cast<QAction *>(sender());
        if (action == m_wireframeAction) {
            documentWidget->setDisplayMode(core::ModelDisplayMode::Wireframe);
        } else if (action == m_shadedAction) {
            documentWidget->setDisplayMode(core::ModelDisplayMode::Shaded);
        } else if (action == m_shadedEdgesAction) {
            documentWidget->setDisplayMode(core::ModelDisplayMode::ShadedWithEdges);
        }
    }
}

void MainWindow::setModelSelectionMode()
{
    if (auto *documentWidget = m_workspaceWidget->currentModelDocumentWidget()) {
        QAction *action = qobject_cast<QAction *>(sender());
        if (action == m_selectShapeAction) {
            documentWidget->setSelectionMode(core::ModelSelectionMode::Shape);
        } else if (action == m_selectFaceAction) {
            documentWidget->setSelectionMode(core::ModelSelectionMode::Face);
        } else if (action == m_selectEdgeAction) {
            documentWidget->setSelectionMode(core::ModelSelectionMode::Edge);
        } else if (action == m_selectVertexAction) {
            documentWidget->setSelectionMode(core::ModelSelectionMode::Vertex);
        }
    }
}

void MainWindow::clearModelSelection()
{
    if (auto *documentWidget = m_workspaceWidget->currentModelDocumentWidget()) {
        documentWidget->clearSelection();
    } else if (m_modelPropertiesPanel != nullptr) {
        m_modelPropertiesPanel->clearSelection();
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

void MainWindow::openPythonTools()
{
    PythonToolsDialog dialog(m_pythonRuntimeManager, m_scriptExecutionManager, this);
    dialog.exec();
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
    if (!filePath.isEmpty()) {
        openPath(filePath);
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

bool MainWindow::saveCurrentScriptAs()
{
    if (m_workspaceWidget == nullptr || !m_workspaceWidget->hasAvailableEditor()) {
        return false;
    }

    ScriptEditorWidget *editor = m_workspaceWidget->currentEditor();
    if (editor == nullptr) {
        return false;
    }

    const QString currentPath = editor->currentFilePath();
    const QString startPath = currentPath.isEmpty() ? m_workspaceManager.fileBrowserRoot() : currentPath;
    const QString filePath = saveScriptPath(startPath, tr("Save File As"));
    if (filePath.isEmpty()) {
        return false;
    }

    const bool saved = m_workspaceWidget->saveCurrentAs(filePath);
    if (saved) {
        if (!currentPath.isEmpty() && currentPath != filePath) {
            m_workspaceManager.replaceRecentFilePath(currentPath, filePath);
        } else {
            m_workspaceManager.addRecentFile(filePath);
        }
    }
    return saved;
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
        filePath = saveScriptPath(m_workspaceManager.fileBrowserRoot(), tr("Save File"));
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
    m_console->appendOutput(prefix, message);
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
    updatePropertiesDockVisibility();
}

void MainWindow::retranslateUi()
{
    const QString filePath = currentScriptFilePath();
    setWindowTitle(filePath.isEmpty() ? tr("PyraQt - Untitled") : tr("PyraQt - %1").arg(filePath));

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
    if (auto *modelViewMenu = findChild<QMenu *>(QStringLiteral("modelViewMenu"))) {
        modelViewMenu->setTitle(tr("Model View"));
    }
    if (auto *displayModeMenu = findChild<QMenu *>(QStringLiteral("displayModeMenu"))) {
        displayModeMenu->setTitle(tr("Display Mode"));
    }
    if (auto *selectionMenu = findChild<QMenu *>(QStringLiteral("selectionMenu"))) {
        selectionMenu->setTitle(tr("Selection Mode"));
    }
    if (auto *languageMenu = findChild<QMenu *>(QStringLiteral("languageMenu"))) {
        languageMenu->setTitle(tr("&Language"));
    }
    if (auto *quitAction = findChild<QAction *>(QStringLiteral("quitAction"))) {
        quitAction->setText(tr("&Quit"));
    }
    if (m_fileToolBar != nullptr) {
        m_fileToolBar->setWindowTitle(tr("File Tools"));
    }
    if (m_scriptToolBar != nullptr) {
        m_scriptToolBar->setWindowTitle(tr("Script Tools"));
    }
    if (m_workspaceToolBar != nullptr) {
        m_workspaceToolBar->setWindowTitle(tr("Workspace Tools"));
    }
    if (m_viewToolBar != nullptr) {
        m_viewToolBar->setWindowTitle(tr("View Tools"));
    }
    if (m_selectionToolBar != nullptr) {
        m_selectionToolBar->setWindowTitle(tr("Selection Tools"));
    }
    if (m_themeToolBar != nullptr) {
        m_themeToolBar->setWindowTitle(tr("Theme Tools"));
    }

    if (m_lightThemeAction != nullptr) {
        m_lightThemeAction->setText(tr("Light"));
        m_lightThemeAction->setToolTip(tr("Switch to light theme"));
        m_lightThemeAction->setStatusTip(tr("Switch to light theme"));
    }
    if (m_darkThemeAction != nullptr) {
        m_darkThemeAction->setText(tr("Dark"));
        m_darkThemeAction->setToolTip(tr("Switch to dark theme"));
        m_darkThemeAction->setStatusTip(tr("Switch to dark theme"));
    }
    if (m_englishAction != nullptr) {
        m_englishAction->setText(tr("English"));
    }
    if (m_chineseAction != nullptr) {
        m_chineseAction->setText(tr("Simplified Chinese"));
    }
    if (m_newScriptAction != nullptr) {
        m_newScriptAction->setText(tr("New File"));
        m_newScriptAction->setToolTip(tr("Create a new file"));
        m_newScriptAction->setStatusTip(tr("Create a new file"));
    }
    if (m_openScriptAction != nullptr) {
        m_openScriptAction->setText(tr("Open File"));
        m_openScriptAction->setToolTip(tr("Open a file"));
        m_openScriptAction->setStatusTip(tr("Open a file"));
    }
    if (m_saveScriptAction != nullptr) {
        m_saveScriptAction->setText(tr("Save File"));
        m_saveScriptAction->setToolTip(tr("Save the current file"));
        m_saveScriptAction->setStatusTip(tr("Save the current file"));
    }
    if (m_saveScriptAsAction != nullptr) {
        m_saveScriptAsAction->setText(tr("Save File As..."));
        m_saveScriptAsAction->setToolTip(tr("Save the current file to a new path"));
        m_saveScriptAsAction->setStatusTip(tr("Save the current file to a new path"));
    }
    if (m_saveAllScriptsAction != nullptr) {
        m_saveAllScriptsAction->setText(tr("Save All"));
        m_saveAllScriptsAction->setToolTip(tr("Save all opened editable files"));
        m_saveAllScriptsAction->setStatusTip(tr("Save all opened editable files"));
    }
    if (m_runScriptAction != nullptr) {
        m_runScriptAction->setText(tr("Run Script"));
        m_runScriptAction->setToolTip(tr("Run the current Python file"));
        m_runScriptAction->setStatusTip(tr("Run the current Python file"));
    }
    if (m_stopScriptAction != nullptr) {
        m_stopScriptAction->setText(tr("Stop Script"));
        m_stopScriptAction->setToolTip(tr("Stop the running script"));
        m_stopScriptAction->setStatusTip(tr("Stop the running script"));
    }
    if (m_closeTabAction != nullptr) {
        m_closeTabAction->setText(tr("Close Tab"));
        m_closeTabAction->setToolTip(tr("Close the current tab"));
        m_closeTabAction->setStatusTip(tr("Close the current tab"));
    }
    if (m_closeOtherTabsAction != nullptr) {
        m_closeOtherTabsAction->setText(tr("Close Other Tabs"));
        m_closeOtherTabsAction->setToolTip(tr("Close all tabs except the current one"));
        m_closeOtherTabsAction->setStatusTip(tr("Close all tabs except the current one"));
    }
    if (m_closeRightTabsAction != nullptr) {
        m_closeRightTabsAction->setText(tr("Close Tabs to the Right"));
        m_closeRightTabsAction->setToolTip(tr("Close all tabs to the right"));
        m_closeRightTabsAction->setStatusTip(tr("Close all tabs to the right"));
    }
    if (m_closeAllTabsAction != nullptr) {
        m_closeAllTabsAction->setText(tr("Close All Tabs"));
        m_closeAllTabsAction->setToolTip(tr("Close all open tabs"));
        m_closeAllTabsAction->setStatusTip(tr("Close all open tabs"));
    }
    if (m_reopenSessionAction != nullptr) {
        m_reopenSessionAction->setText(tr("Reopen Last Session"));
        m_reopenSessionAction->setToolTip(tr("Reopen the last session"));
        m_reopenSessionAction->setStatusTip(tr("Reopen the last session"));
    }
    if (m_settingsAction != nullptr) {
        m_settingsAction->setText(tr("Settings"));
        m_settingsAction->setToolTip(tr("Open application settings"));
        m_settingsAction->setStatusTip(tr("Open application settings"));
    }
    if (m_pythonToolsAction != nullptr) {
        m_pythonToolsAction->setText(tr("Python Tools"));
        m_pythonToolsAction->setToolTip(tr("Open Python tools"));
        m_pythonToolsAction->setStatusTip(tr("Open Python tools"));
    }
    if (m_chooseFileBrowserRootAction != nullptr) {
        m_chooseFileBrowserRootAction->setText(tr("Set File Browser Root"));
        m_chooseFileBrowserRootAction->setToolTip(tr("Choose the file browser root folder"));
        m_chooseFileBrowserRootAction->setStatusTip(tr("Choose the file browser root folder"));
    }
    if (m_commandPaletteAction != nullptr) {
        m_commandPaletteAction->setText(tr("Command Palette"));
        m_commandPaletteAction->setToolTip(tr("Open the command palette"));
        m_commandPaletteAction->setStatusTip(tr("Open the command palette"));
    }
    if (m_checkUpdatesAction != nullptr) {
        m_checkUpdatesAction->setText(tr("Check for Updates"));
        m_checkUpdatesAction->setToolTip(tr("Check for available updates"));
        m_checkUpdatesAction->setStatusTip(tr("Check for available updates"));
    }
    if (m_fitAllAction != nullptr) {
        m_fitAllAction->setText(tr("Fit All"));
        m_fitAllAction->setToolTip(tr("Fit the current model in view"));
        m_fitAllAction->setStatusTip(tr("Fit the current model in view"));
    }
    if (m_viewFrontAction != nullptr) {
        m_viewFrontAction->setText(tr("View Front"));
        m_viewFrontAction->setToolTip(tr("Switch to front view"));
        m_viewFrontAction->setStatusTip(tr("Switch to front view"));
    }
    if (m_viewBackAction != nullptr) {
        m_viewBackAction->setText(tr("View Back"));
        m_viewBackAction->setToolTip(tr("Switch to back view"));
        m_viewBackAction->setStatusTip(tr("Switch to back view"));
    }
    if (m_viewLeftAction != nullptr) {
        m_viewLeftAction->setText(tr("View Left"));
        m_viewLeftAction->setToolTip(tr("Switch to left view"));
        m_viewLeftAction->setStatusTip(tr("Switch to left view"));
    }
    if (m_viewRightAction != nullptr) {
        m_viewRightAction->setText(tr("View Right"));
        m_viewRightAction->setToolTip(tr("Switch to right view"));
        m_viewRightAction->setStatusTip(tr("Switch to right view"));
    }
    if (m_viewTopAction != nullptr) {
        m_viewTopAction->setText(tr("View Top"));
        m_viewTopAction->setToolTip(tr("Switch to top view"));
        m_viewTopAction->setStatusTip(tr("Switch to top view"));
    }
    if (m_viewBottomAction != nullptr) {
        m_viewBottomAction->setText(tr("View Bottom"));
        m_viewBottomAction->setToolTip(tr("Switch to bottom view"));
        m_viewBottomAction->setStatusTip(tr("Switch to bottom view"));
    }
    if (m_viewIsoAction != nullptr) {
        m_viewIsoAction->setText(tr("View Isometric"));
        m_viewIsoAction->setToolTip(tr("Switch to isometric view"));
        m_viewIsoAction->setStatusTip(tr("Switch to isometric view"));
    }
    if (m_wireframeAction != nullptr) {
        m_wireframeAction->setText(tr("Wireframe"));
        m_wireframeAction->setToolTip(tr("Set model display mode to wireframe"));
        m_wireframeAction->setStatusTip(tr("Set model display mode to wireframe"));
    }
    if (m_shadedAction != nullptr) {
        m_shadedAction->setText(tr("Shaded"));
        m_shadedAction->setToolTip(tr("Set model display mode to shaded"));
        m_shadedAction->setStatusTip(tr("Set model display mode to shaded"));
    }
    if (m_shadedEdgesAction != nullptr) {
        m_shadedEdgesAction->setText(tr("Shaded With Edges"));
        m_shadedEdgesAction->setToolTip(tr("Set model display mode to shaded with edges"));
        m_shadedEdgesAction->setStatusTip(tr("Set model display mode to shaded with edges"));
    }
    if (m_selectShapeAction != nullptr) {
        m_selectShapeAction->setText(tr("Select Shape"));
        m_selectShapeAction->setToolTip(tr("Select entire shapes"));
        m_selectShapeAction->setStatusTip(tr("Select entire shapes"));
    }
    if (m_selectFaceAction != nullptr) {
        m_selectFaceAction->setText(tr("Select Face"));
        m_selectFaceAction->setToolTip(tr("Select model faces"));
        m_selectFaceAction->setStatusTip(tr("Select model faces"));
    }
    if (m_selectEdgeAction != nullptr) {
        m_selectEdgeAction->setText(tr("Select Edge"));
        m_selectEdgeAction->setToolTip(tr("Select model edges"));
        m_selectEdgeAction->setStatusTip(tr("Select model edges"));
    }
    if (m_selectVertexAction != nullptr) {
        m_selectVertexAction->setText(tr("Select Vertex"));
        m_selectVertexAction->setToolTip(tr("Select model vertices"));
        m_selectVertexAction->setStatusTip(tr("Select model vertices"));
    }
    if (m_clearSelectionAction != nullptr) {
        m_clearSelectionAction->setText(tr("Clear Selection"));
        m_clearSelectionAction->setToolTip(tr("Clear the current model selection"));
        m_clearSelectionAction->setStatusTip(tr("Clear the current model selection"));
    }
    configureActionPresentation();
    if (m_pythonStatusLabel != nullptr) {
        m_pythonStatusLabel->setText(tr("Python: %1").arg(m_pythonRuntimeManager.pythonVersion()));
    }
    if (m_documentStatusLabel != nullptr && m_workspaceWidget != nullptr) {
        if (ModelDocumentWidget *documentWidget = m_workspaceWidget->currentModelDocumentWidget()) {
            m_documentStatusLabel->setText(documentWidget->document().isValid ? tr("Document: Model Loaded") : tr("Document: Load Failed"));
        } else if (m_workspaceWidget->currentDocumentKind() == EditorWorkspaceWidget::DocumentKind::PreviewUnavailable) {
            m_documentStatusLabel->setText(tr("Document: Preview Unavailable"));
        } else {
            ScriptEditorWidget *editor = m_workspaceWidget->currentEditor();
            m_documentStatusLabel->setText(editor != nullptr && editor->isModified() ? tr("Document: Modified") : tr("Document: Saved"));
        }
    }
    if (m_fileStatusLabel != nullptr) {
        m_fileStatusLabel->setText(filePath.isEmpty() ? tr("File: Untitled") : tr("File: %1").arg(QFileInfo(filePath).fileName()));
    }

    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::applyActionIcons()
{
    const QString themeName = m_themeManager.currentTheme();
    const auto setActionIcon = [themeName](QAction *action, const QString &iconName) {
        if (action == nullptr) {
            return;
        }
        action->setIcon(themedSvgIcon(iconName, themeName));
    };

    setActionIcon(m_newScriptAction, QStringLiteral("new-file"));
    setActionIcon(m_openScriptAction, QStringLiteral("open-file"));
    setActionIcon(m_saveScriptAction, QStringLiteral("save"));
    setActionIcon(m_saveScriptAsAction, QStringLiteral("save"));
    setActionIcon(m_saveAllScriptsAction, QStringLiteral("save-all"));
    setActionIcon(m_runScriptAction, QStringLiteral("run"));
    setActionIcon(m_stopScriptAction, QStringLiteral("stop"));
    setActionIcon(m_closeTabAction, QStringLiteral("close-tab"));
    setActionIcon(m_closeOtherTabsAction, QStringLiteral("close-tab"));
    setActionIcon(m_closeRightTabsAction, QStringLiteral("close-tab"));
    setActionIcon(m_closeAllTabsAction, QStringLiteral("close-tab"));
    setActionIcon(m_reopenSessionAction, QStringLiteral("reopen-session"));
    setActionIcon(m_settingsAction, QStringLiteral("settings"));
    setActionIcon(m_pythonToolsAction, QStringLiteral("python-tools"));
    setActionIcon(m_chooseFileBrowserRootAction, QStringLiteral("file-browser-root"));
    setActionIcon(m_commandPaletteAction, QStringLiteral("command-palette"));
    setActionIcon(m_checkUpdatesAction, QStringLiteral("update"));
    setActionIcon(m_fitAllAction, QStringLiteral("fit-all"));
    setActionIcon(m_viewFrontAction, QStringLiteral("view-front"));
    setActionIcon(m_viewBackAction, QStringLiteral("view-back"));
    setActionIcon(m_viewLeftAction, QStringLiteral("view-left"));
    setActionIcon(m_viewRightAction, QStringLiteral("view-right"));
    setActionIcon(m_viewTopAction, QStringLiteral("view-top"));
    setActionIcon(m_viewBottomAction, QStringLiteral("view-bottom"));
    setActionIcon(m_viewIsoAction, QStringLiteral("view-iso"));
    setActionIcon(m_wireframeAction, QStringLiteral("wireframe"));
    setActionIcon(m_shadedAction, QStringLiteral("shaded"));
    setActionIcon(m_shadedEdgesAction, QStringLiteral("shaded-edges"));
    setActionIcon(m_selectShapeAction, QStringLiteral("select-shape"));
    setActionIcon(m_selectFaceAction, QStringLiteral("select-face"));
    setActionIcon(m_selectEdgeAction, QStringLiteral("select-edge"));
    setActionIcon(m_selectVertexAction, QStringLiteral("select-vertex"));
    setActionIcon(m_clearSelectionAction, QStringLiteral("clear-selection"));
    setActionIcon(m_lightThemeAction, QStringLiteral("sun"));
    setActionIcon(m_darkThemeAction, QStringLiteral("moon"));
}

void MainWindow::configureActionPresentation()
{
    const auto configureAction = [](QAction *action) {
        if (action == nullptr) {
            return;
        }
        action->setIconVisibleInMenu(true);
        if (action->shortcut().isEmpty()) {
            return;
        }
        const QString shortcutText = action->shortcut().toString(QKeySequence::NativeText);
        if (!shortcutText.isEmpty()) {
            const QString baseToolTip = action->statusTip().isEmpty() ? action->toolTip() : action->statusTip();
            action->setToolTip(QStringLiteral("%1 (%2)").arg(baseToolTip, shortcutText));
        }
    };

    for (QAction *action : {m_newScriptAction, m_openScriptAction, m_saveScriptAction, m_saveScriptAsAction, m_saveAllScriptsAction,
             m_runScriptAction, m_stopScriptAction, m_closeTabAction, m_closeOtherTabsAction, m_closeRightTabsAction, m_closeAllTabsAction,
             m_reopenSessionAction, m_settingsAction, m_pythonToolsAction, m_chooseFileBrowserRootAction, m_commandPaletteAction,
             m_checkUpdatesAction, m_fitAllAction, m_viewFrontAction, m_viewBackAction, m_viewLeftAction, m_viewRightAction,
             m_viewTopAction, m_viewBottomAction, m_viewIsoAction, m_wireframeAction, m_shadedAction, m_shadedEdgesAction,
             m_selectShapeAction, m_selectFaceAction, m_selectEdgeAction, m_selectVertexAction, m_clearSelectionAction,
             m_lightThemeAction, m_darkThemeAction}) {
        configureAction(action);
    }
}

void MainWindow::configureToolbar()
{
    for (QToolBar *toolBar : {m_fileToolBar, m_scriptToolBar, m_workspaceToolBar, m_viewToolBar, m_selectionToolBar, m_themeToolBar}) {
        if (toolBar == nullptr) {
            continue;
        }
        toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        toolBar->setIconSize(QSize(20, 20));
        toolBar->setMovable(true);
        toolBar->setFloatable(true);
    }
}

void MainWindow::updatePropertiesDockVisibility()
{
    if (m_propertiesDock == nullptr || m_workspaceWidget == nullptr) {
        return;
    }

    const bool shouldShow = m_workspaceWidget->currentDocumentKind() == EditorWorkspaceWidget::DocumentKind::Model;
    m_propertiesDock->setVisible(shouldShow);
}

QToolBar *MainWindow::createGroupedToolBar(const QString &objectName, const QList<QAction *> &actions, const QString &title)
{
    auto *toolBar = addToolBar(title);
    toolBar->setObjectName(objectName);
    toolBar->setWindowTitle(title);
    for (QAction *action : actions) {
        if (action != nullptr) {
            toolBar->addAction(action);
        }
    }
    return toolBar;
}

void MainWindow::updateTabActionStates()
{
    const bool hasWorkspace = m_workspaceWidget != nullptr;
    const int tabCount = hasWorkspace ? m_workspaceWidget->editorCount() : 0;
    const bool hasTabs = tabCount > 0;
    const bool hasMultipleTabs = tabCount > 1;
    const bool hasRightTabs = hasWorkspace && m_workspaceWidget->hasEditorsToRight();
    const bool hasEditableEditor = hasWorkspace && m_workspaceWidget->hasAvailableEditor();
    const bool canRunCurrent = hasWorkspace && m_workspaceWidget->currentFileRunnable() && !m_scriptExecutionManager.isRunning();
    const bool canStopCurrent = m_scriptExecutionManager.isRunning();

    if (m_closeTabAction != nullptr) {
        m_closeTabAction->setEnabled(hasTabs);
    }
    if (m_closeOtherTabsAction != nullptr) {
        m_closeOtherTabsAction->setEnabled(hasMultipleTabs);
    }
    if (m_closeRightTabsAction != nullptr) {
        m_closeRightTabsAction->setEnabled(hasRightTabs);
    }
    if (m_closeAllTabsAction != nullptr) {
        m_closeAllTabsAction->setEnabled(hasTabs);
    }
    if (m_saveScriptAction != nullptr) {
        m_saveScriptAction->setEnabled(hasEditableEditor);
    }
    if (m_saveScriptAsAction != nullptr) {
        m_saveScriptAsAction->setEnabled(hasEditableEditor);
    }
    if (m_saveAllScriptsAction != nullptr) {
        m_saveAllScriptsAction->setEnabled(hasWorkspace);
    }
    if (m_runScriptAction != nullptr) {
        m_runScriptAction->setEnabled(canRunCurrent);
    }
    if (m_stopScriptAction != nullptr) {
        m_stopScriptAction->setEnabled(canStopCurrent);
    }
}

void MainWindow::updateModelActionStates()
{
    const bool hasModel = m_workspaceWidget != nullptr && m_workspaceWidget->currentModelDocumentWidget() != nullptr;
    const core::ModelDocument document = hasModel ? m_workspaceWidget->currentModelDocument() : core::ModelDocument{};
    for (QAction *action : {m_fitAllAction, m_viewFrontAction, m_viewBackAction, m_viewLeftAction, m_viewRightAction, m_viewTopAction,
             m_viewBottomAction, m_viewIsoAction, m_wireframeAction, m_shadedAction, m_shadedEdgesAction,
             m_selectShapeAction, m_selectFaceAction, m_selectEdgeAction, m_selectVertexAction, m_clearSelectionAction}) {
        if (action != nullptr) {
            action->setEnabled(hasModel);
        }
    }
    if (m_wireframeAction != nullptr) {
        m_wireframeAction->setChecked(document.displayMode == core::ModelDisplayMode::Wireframe);
    }
    if (m_shadedAction != nullptr) {
        m_shadedAction->setChecked(document.displayMode == core::ModelDisplayMode::Shaded);
    }
    if (m_shadedEdgesAction != nullptr) {
        m_shadedEdgesAction->setChecked(document.displayMode == core::ModelDisplayMode::ShadedWithEdges);
    }
    if (m_selectShapeAction != nullptr) {
        m_selectShapeAction->setChecked(document.selectionMode == core::ModelSelectionMode::Shape);
    }
    if (m_selectFaceAction != nullptr) {
        m_selectFaceAction->setChecked(document.selectionMode == core::ModelSelectionMode::Face);
    }
    if (m_selectEdgeAction != nullptr) {
        m_selectEdgeAction->setChecked(document.selectionMode == core::ModelSelectionMode::Edge);
    }
    if (m_selectVertexAction != nullptr) {
        m_selectVertexAction->setChecked(document.selectionMode == core::ModelSelectionMode::Vertex);
    }
}

bool MainWindow::openPath(const QString &filePath, const bool addToRecent)
{
    if (filePath.isEmpty() || m_workspaceWidget == nullptr) {
        return false;
    }

    const bool opened = m_workspaceWidget->openPath(filePath);
    if (opened && addToRecent) {
        m_workspaceManager.addRecentFile(filePath);
    }
    if (opened) {
        updatePropertiesDockVisibility();
    }
    return opened;
}

void MainWindow::renamePathFromFileBrowser(const QString &path)
{
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, tr("Rename"), tr("The selected path no longer exists."));
        return;
    }

    if (fileInfo.isFile() && m_workspaceWidget != nullptr) {
        ScriptEditorWidget *editor = m_workspaceWidget->currentEditor();
        if (editor != nullptr && editor->currentFilePath() == fileInfo.absoluteFilePath() && editor->isModified()) {
            QMessageBox::warning(this, tr("Rename"), tr("Save the current script before renaming it."));
            return;
        }
    }

    bool accepted = false;
    const QString newName = QInputDialog::getText(
        this,
        tr("Rename"),
        fileInfo.isDir() ? tr("Folder name") : tr("File name"),
        QLineEdit::Normal,
        fileInfo.fileName(),
        &accepted);
    if (!accepted) {
        return;
    }

    const QString trimmedName = newName.trimmed();
    if (trimmedName.isEmpty() || trimmedName == fileInfo.fileName()) {
        return;
    }
    if (trimmedName.contains(QDir::separator())) {
        QMessageBox::warning(this, tr("Rename"), tr("Only renaming within the same folder is supported."));
        return;
    }

    const QString newPath = fileInfo.dir().absoluteFilePath(trimmedName);
    if (QFileInfo::exists(newPath)) {
        QMessageBox::warning(this, tr("Rename"), tr("A file or folder with that name already exists."));
        return;
    }

    bool renamed = false;
    if (fileInfo.isDir()) {
        QDir parentDir = fileInfo.dir();
        renamed = parentDir.rename(fileInfo.fileName(), trimmedName);
    } else {
        QFile file(path);
        renamed = file.rename(newPath);
    }

    if (!renamed) {
        QMessageBox::warning(this, tr("Rename"), tr("Failed to rename the selected path."));
        return;
    }

    if (m_workspaceWidget != nullptr && m_workspaceWidget->hasOpenPath(fileInfo.absoluteFilePath())) {
        m_workspaceWidget->renameOpenPath(fileInfo.absoluteFilePath(), newPath);
    }
    m_workspaceManager.replaceRecentFilePath(fileInfo.absoluteFilePath(), newPath);
    updateFileBrowserRootAfterPathChange(fileInfo.absoluteFilePath(), newPath);
    updatePropertiesDockVisibility();
}

void MainWindow::deletePathFromFileBrowser(const QString &path)
{
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, tr("Delete"), tr("The selected path no longer exists."));
        return;
    }
    if (!confirmDeletePath(path) || !confirmDeleteOpenEditors(path)) {
        return;
    }

    const QStringList openPaths = openPathsUnder(path);
    if (!removePathRecursively(path)) {
        QMessageBox::warning(this, tr("Delete"), tr("Failed to delete the selected path."));
        return;
    }

    if (m_workspaceWidget != nullptr) {
        for (const QString &openPath : openPaths) {
            if (m_workspaceWidget->hasOpenPath(openPath)) {
                m_workspaceWidget->closePath(openPath);
            }
        }
    }
    for (const QString &openPath : openPaths) {
        m_workspaceManager.removeRecentFile(openPath);
    }
    m_workspaceManager.removeRecentFile(fileInfo.absoluteFilePath());
    updateFileBrowserRootAfterPathChange(fileInfo.absoluteFilePath());
    updatePropertiesDockVisibility();
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

void MainWindow::updateFileBrowserRootAfterPathChange(const QString &oldPath, const QString &newPath)
{
    const QString currentRoot = m_workspaceManager.fileBrowserRoot();
    if (currentRoot.isEmpty()) {
        return;
    }

    const QDir currentRootDir(currentRoot);
    const QString oldAbsolutePath = QFileInfo(oldPath).absoluteFilePath();
    const bool sameRoot = currentRootDir.absolutePath() == oldAbsolutePath;
    const bool insideOldRoot = currentRoot.startsWith(oldAbsolutePath + QDir::separator());
    if (!sameRoot && !insideOldRoot) {
        return;
    }

    QString updatedRoot;
    if (!newPath.isEmpty()) {
        if (sameRoot) {
            updatedRoot = QFileInfo(newPath).absoluteFilePath();
        } else {
            updatedRoot = currentRoot;
            updatedRoot.replace(0, oldAbsolutePath.size(), QFileInfo(newPath).absoluteFilePath());
        }
    }

    QFileInfo updatedInfo(updatedRoot);
    if (updatedRoot.isEmpty() || !updatedInfo.exists()) {
        QDir fallback = QFileInfo(oldAbsolutePath).dir();
        while (!fallback.exists() && !fallback.isRoot()) {
            fallback.cdUp();
        }
        updatedRoot = fallback.exists() ? fallback.absolutePath() : QDir::homePath();
    }

    m_workspaceManager.setFileBrowserRoot(updatedRoot);
    const bool saved = m_configManager.save();
    if (!saved) {
        m_logManager.warning(tr("Failed to save file browser root."));
    }
}

bool MainWindow::confirmDeletePath(const QString &path)
{
    const QFileInfo fileInfo(path);
    const QString title = tr("Delete");
    const QString message = fileInfo.isDir()
        ? tr("Delete folder \"%1\" and all of its contents?").arg(fileInfo.fileName())
        : tr("Delete file \"%1\"?").arg(fileInfo.fileName());
    return QMessageBox::question(this, title, message, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

bool MainWindow::confirmDeleteOpenEditors(const QString &path)
{
    if (m_workspaceWidget == nullptr) {
        return true;
    }

    const QStringList openPaths = openPathsUnder(path);
    for (const QString &openPath : openPaths) {
        ScriptEditorWidget *editor = nullptr;
        for (int tabIndex = 0; tabIndex < m_workspaceWidget->editorCount(); ++tabIndex) {
            ScriptEditorWidget *candidate = m_workspaceWidget->editorAt(tabIndex);
            if (candidate != nullptr && candidate->currentFilePath() == openPath) {
                editor = candidate;
                break;
            }
        }
        if (editor != nullptr && editor->isModified()) {
            const QString message = tr("The open script \"%1\" has unsaved changes. Delete it and close the tab anyway?")
                                        .arg(QFileInfo(openPath).fileName());
            if (QMessageBox::question(this, tr("Delete Open Script"), message, QMessageBox::Yes | QMessageBox::No)
                != QMessageBox::Yes) {
                return false;
            }
        }
    }
    return true;
}

QString MainWindow::saveScriptPath(const QString &initialPath, const QString &title)
{
    const QFileInfo fileInfo(initialPath);
    const QString startPath = fileInfo.exists() && fileInfo.isFile() ? fileInfo.absoluteFilePath()
                                                                     : (fileInfo.exists() ? fileInfo.absoluteFilePath() : initialPath);
    const QString suffix = fileInfo.isFile() && !fileInfo.suffix().isEmpty() ? fileInfo.suffix().toLower() : QStringLiteral("txt");
    const bool untitledEditor = m_workspaceWidget != nullptr
        && m_workspaceWidget->currentEditor() != nullptr
        && m_workspaceWidget->currentEditor()->currentFilePath().isEmpty();
    const QString filter = untitledEditor
        ? tr("Text Files (*.txt *.md *.json *.xml *.yaml *.yml *.toml *.ini *.cfg *.conf *.log *.csv *.py *.cpp *.c *.h *.hpp *.qml *.js *.ts *.html *.css);;All Files (*)")
        : (suffix == QStringLiteral("py")
            ? tr("Python Files (*.py);;All Files (*)")
            : tr("%1 Files (*.%2);;All Files (*)").arg(suffix.toUpper(), suffix));
    return getThemedSaveFileName(
        {
            title,
            startPath,
            filter,
            suffix,
            QFileDialog::AnyFile,
            QFileDialog::AcceptSave,
        },
        this);
}

QStringList MainWindow::openPathsUnder(const QString &path) const
{
    QStringList paths;
    if (m_workspaceWidget == nullptr) {
        return paths;
    }

    const QFileInfo targetInfo(path);
    const QString targetPath = targetInfo.absoluteFilePath();
    for (const QString &openPath : m_workspaceWidget->openFilePaths()) {
        if (openPath == targetPath || openPath.startsWith(targetPath + QDir::separator())) {
            paths.push_back(openPath);
        }
    }
    return paths;
}

bool MainWindow::removePathRecursively(const QString &path)
{
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        return true;
    }
    if (fileInfo.isDir()) {
        QDir dir(path);
        return dir.removeRecursively();
    }
    return QFile::remove(path);
}

} // namespace pyraqt::ui

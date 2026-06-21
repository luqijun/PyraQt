#pragma once

#include <QMainWindow>

class QAction;
class QCloseEvent;
class QLabel;
class QMenu;
class QPlainTextEdit;
class QDockWidget;

namespace pyraqt::core {
class CommandManager;
class ConfigManager;
class CrashRecoveryManager;
class I18nManager;
class LogManager;
class ModelImportManager;
class PluginManager;
class PythonRuntimeManager;
class ScriptExecutionManager;
class ThemeManager;
class UpdateManager;
class WorkspaceManager;
} // namespace pyraqt::core

namespace pyraqt::ui {

class CommandPaletteDialog;
class EditorWorkspaceWidget;
class FileBrowserPanel;
class ModelDocumentWidget;
class ModelPropertiesPanel;
class PluginManagerPanel;
class PythonConsoleWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(
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
        QWidget *parent = nullptr);

    void saveSession();

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void createCentralEditor();
    void createDocks();
    void createMenus();
    void createStatusBar();
    void createScriptActions();
    void refreshRecentFilesMenu();
    void registerBuiltInCommands();
    void updateModelActionStates();
    bool openPath(const QString &filePath, bool addToRecent = true);
    void runCurrentScript();
    void fitModelView();
    void setModelViewPreset();
    void setModelDisplayMode();
    void setModelSelectionMode();
    void clearModelSelection();
    void checkForUpdates();
    void openSettings();
    void openPythonTools();
    void reopenLastSession();
    void chooseFileBrowserRoot();
    void renamePathFromFileBrowser(const QString &path);
    void deletePathFromFileBrowser(const QString &path);
    void openRecentFile();
    bool saveAllScripts();
    void showRecoveryPromptIfNeeded();
    bool saveCurrentScriptIfNeeded();
    bool saveCurrentScriptAs();
    QString currentScriptFilePath() const;
    void openCommandPalette();
    void appendConsoleLine(const QString &prefix, const QString &message);
    void restoreSession();
    void retranslateUi();
    void updateFileBrowserRootAfterPathChange(const QString &oldPath, const QString &newPath = QString());
    bool confirmDeletePath(const QString &path);
    bool confirmDeleteOpenEditors(const QString &path);
    [[nodiscard]] QString saveScriptPath(const QString &initialPath, const QString &title);
    [[nodiscard]] QStringList openPathsUnder(const QString &path) const;
    bool removePathRecursively(const QString &path);
    QDockWidget *createTextDock(const QString &objectName, const QString &title, const QString &body);

    core::ConfigManager &m_configManager;
    core::LogManager &m_logManager;
    core::ModelImportManager &m_modelImportManager;
    core::ThemeManager &m_themeManager;
    core::I18nManager &m_i18nManager;
    core::PythonRuntimeManager &m_pythonRuntimeManager;
    core::ScriptExecutionManager &m_scriptExecutionManager;
    core::CommandManager &m_commandManager;
    core::PluginManager &m_pluginManager;
    core::WorkspaceManager &m_workspaceManager;
    core::UpdateManager &m_updateManager;
    core::CrashRecoveryManager &m_crashRecoveryManager;

    FileBrowserPanel *m_fileBrowserPanel = nullptr;
    ModelPropertiesPanel *m_modelPropertiesPanel = nullptr;
    PluginManagerPanel *m_pluginManagerPanel = nullptr;
    EditorWorkspaceWidget *m_workspaceWidget = nullptr;
    PythonConsoleWidget *m_console = nullptr;
    QLabel *m_pythonStatusLabel = nullptr;
    QLabel *m_documentStatusLabel = nullptr;
    QLabel *m_fileStatusLabel = nullptr;
    QLabel *m_cursorStatusLabel = nullptr;
    QMenu *m_recentFilesMenu = nullptr;
    QAction *m_commandPaletteAction = nullptr;
    QAction *m_lightThemeAction = nullptr;
    QAction *m_darkThemeAction = nullptr;
    QAction *m_englishAction = nullptr;
    QAction *m_chineseAction = nullptr;
    QAction *m_newScriptAction = nullptr;
    QAction *m_openScriptAction = nullptr;
    QAction *m_saveScriptAction = nullptr;
    QAction *m_saveScriptAsAction = nullptr;
    QAction *m_saveAllScriptsAction = nullptr;
    QAction *m_runScriptAction = nullptr;
    QAction *m_stopScriptAction = nullptr;
    QAction *m_closeTabAction = nullptr;
    QAction *m_closeOtherTabsAction = nullptr;
    QAction *m_reopenSessionAction = nullptr;
    QAction *m_settingsAction = nullptr;
    QAction *m_pythonToolsAction = nullptr;
    QAction *m_chooseFileBrowserRootAction = nullptr;
    QAction *m_checkUpdatesAction = nullptr;
    QAction *m_fitAllAction = nullptr;
    QAction *m_viewFrontAction = nullptr;
    QAction *m_viewBackAction = nullptr;
    QAction *m_viewLeftAction = nullptr;
    QAction *m_viewRightAction = nullptr;
    QAction *m_viewTopAction = nullptr;
    QAction *m_viewBottomAction = nullptr;
    QAction *m_viewIsoAction = nullptr;
    QAction *m_wireframeAction = nullptr;
    QAction *m_shadedAction = nullptr;
    QAction *m_shadedEdgesAction = nullptr;
    QAction *m_selectShapeAction = nullptr;
    QAction *m_selectFaceAction = nullptr;
    QAction *m_selectEdgeAction = nullptr;
    QAction *m_selectVertexAction = nullptr;
    QAction *m_clearSelectionAction = nullptr;
    bool m_sessionSaved = false;
};

} // namespace pyraqt::ui

#pragma once

#include <QMainWindow>

class QAction;
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
class PluginManagerPanel;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(
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
        QWidget *parent = nullptr);

    void saveSession();

protected:
    void changeEvent(QEvent *event) override;

private:
    void createCentralEditor();
    void createDocks();
    void createMenus();
    void createStatusBar();
    void createScriptActions();
    void refreshRecentFilesMenu();
    void registerBuiltInCommands();
    void runCurrentScript();
    void checkForUpdates();
    void openSettings();
    void reopenLastSession();
    void chooseFileBrowserRoot();
    void openRecentFile();
    bool saveAllScripts();
    void showRecoveryPromptIfNeeded();
    bool saveCurrentScriptIfNeeded();
    QString currentScriptFilePath() const;
    void openCommandPalette();
    void appendConsoleLine(const QString &prefix, const QString &message);
    void restoreSession();
    void retranslateUi();
    QDockWidget *createTextDock(const QString &objectName, const QString &title, const QString &body);

    core::ConfigManager &m_configManager;
    core::LogManager &m_logManager;
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
    PluginManagerPanel *m_pluginManagerPanel = nullptr;
    EditorWorkspaceWidget *m_workspaceWidget = nullptr;
    QPlainTextEdit *m_console = nullptr;
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
    QAction *m_saveAllScriptsAction = nullptr;
    QAction *m_runScriptAction = nullptr;
    QAction *m_stopScriptAction = nullptr;
    QAction *m_closeTabAction = nullptr;
    QAction *m_closeOtherTabsAction = nullptr;
    QAction *m_reopenSessionAction = nullptr;
    QAction *m_settingsAction = nullptr;
    QAction *m_chooseFileBrowserRootAction = nullptr;
    QAction *m_checkUpdatesAction = nullptr;
};

} // namespace pyraqt::ui

#pragma once

#include <QObject>
#include <memory>

class QApplication;

namespace pyraqt::core {
class CommandManager;
class ConfigManager;
class I18nManager;
class LogManager;
class PyraApiBridge;
class PluginManager;
class PythonRuntimeManager;
class CrashRecoveryManager;
class ScriptExecutionManager;
class ThemeManager;
class UpdateManager;
class WorkspaceManager;
} // namespace pyraqt::core

namespace pyraqt::ui {
class MainWindow;
} // namespace pyraqt::ui

namespace pyraqt::app {

class Application final : public QObject {
    Q_OBJECT

public:
    explicit Application(QApplication &qtApplication, QObject *parent = nullptr);
    ~Application() override;

    bool initialize();
    void show();
    void shutdown();

private:
    QApplication &m_qtApplication;
    std::unique_ptr<core::ConfigManager> m_configManager;
    std::unique_ptr<core::LogManager> m_logManager;
    std::unique_ptr<core::ThemeManager> m_themeManager;
    std::unique_ptr<core::I18nManager> m_i18nManager;
    std::unique_ptr<core::PythonRuntimeManager> m_pythonRuntimeManager;
    std::unique_ptr<core::PyraApiBridge> m_pyraApiBridge;
    std::unique_ptr<core::ScriptExecutionManager> m_scriptExecutionManager;
    std::unique_ptr<core::CommandManager> m_commandManager;
    std::unique_ptr<core::PluginManager> m_pluginManager;
    std::unique_ptr<core::CrashRecoveryManager> m_crashRecoveryManager;
    std::unique_ptr<core::UpdateManager> m_updateManager;
    std::unique_ptr<core::WorkspaceManager> m_workspaceManager;
    std::unique_ptr<ui::MainWindow> m_mainWindow;
};

} // namespace pyraqt::app

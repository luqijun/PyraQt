#include "app/application.h"

#include "core/command/command_manager.h"
#include "core/config/config_manager.h"
#include "core/i18n/i18n_manager.h"
#include "core/logging/log_manager.h"
#include "core/modeling/model_import_manager.h"
#include "core/plugin/plugin_manager.h"
#include "core/runtime/crash_recovery_manager.h"
#include "core/scripting/pyra_api_bridge.h"
#include "core/scripting/python_runtime_manager.h"
#include "core/scripting/script_execution_manager.h"
#include "core/theme/theme_manager.h"
#include "core/update/update_manager.h"
#include "core/workspace/workspace_manager.h"
#include "ui/mainwindow/main_window.h"

#include <QApplication>

namespace pyraqt::app {

Application::Application(QApplication &qtApplication, QObject *parent)
    : QObject(parent)
    , m_qtApplication(qtApplication)
{
}

Application::~Application() = default;

bool Application::initialize()
{
    m_qtApplication.setOrganizationName(QStringLiteral("PyraQt"));
    m_qtApplication.setOrganizationDomain(QStringLiteral("pyraqt.local"));
    m_qtApplication.setApplicationName(QStringLiteral("PyraQt"));
    m_qtApplication.setApplicationVersion(QStringLiteral("0.1.0"));

    m_configManager = std::make_unique<core::ConfigManager>();
    m_logManager = std::make_unique<core::LogManager>();
    m_modelImportManager = std::make_unique<core::ModelImportManager>();
    m_themeManager = std::make_unique<core::ThemeManager>(m_qtApplication);
    m_i18nManager = std::make_unique<core::I18nManager>(m_qtApplication);
    m_pythonRuntimeManager = std::make_unique<core::PythonRuntimeManager>(*m_configManager);
    m_pyraApiBridge = std::make_unique<core::PyraApiBridge>(*m_pythonRuntimeManager, *m_logManager);
    m_scriptExecutionManager = std::make_unique<core::ScriptExecutionManager>(*m_pythonRuntimeManager, *m_pyraApiBridge);
    m_commandManager = std::make_unique<core::CommandManager>();
    m_crashRecoveryManager = std::make_unique<core::CrashRecoveryManager>(*m_configManager, *m_logManager);
    m_updateManager = std::make_unique<core::UpdateManager>(*m_configManager, *m_logManager);
    m_workspaceManager = std::make_unique<core::WorkspaceManager>(*m_configManager);
    m_pluginManager = std::make_unique<core::PluginManager>(
        *m_commandManager,
        *m_configManager,
        *m_logManager,
        *m_scriptExecutionManager);

    const bool configLoaded = m_configManager->load();
    const bool loggingReady = m_logManager->initialize();
    m_crashRecoveryManager->markAppStarted();

    m_i18nManager->setLocale(m_configManager->value(QStringLiteral("locale"), QStringLiteral("zh_CN")).toString());
    m_themeManager->setTheme(m_configManager->value(QStringLiteral("theme"), QStringLiteral("light")).toString());

    m_mainWindow = std::make_unique<ui::MainWindow>(
        *m_configManager,
        *m_logManager,
        *m_modelImportManager,
        *m_themeManager,
        *m_i18nManager,
        *m_pythonRuntimeManager,
        *m_scriptExecutionManager,
        *m_commandManager,
        *m_pluginManager,
        *m_workspaceManager,
        *m_updateManager,
        *m_crashRecoveryManager);

    m_pluginManager->scanPlugins();
    m_pluginManager->loadEnabledPlugins();

    QObject::connect(m_themeManager.get(), &core::ThemeManager::themeChanged, m_configManager.get(),
        [this](const QString &themeName) {
            m_configManager->setValue(QStringLiteral("theme"), themeName);
            const bool saved = m_configManager->save();
            if (!saved) {
                m_logManager->warning(QStringLiteral("Failed to persist theme preference"));
            }
        });

    QObject::connect(m_i18nManager.get(), &core::I18nManager::localeChanged, m_configManager.get(),
        [this](const QString &localeName) {
            m_configManager->setValue(QStringLiteral("locale"), localeName);
            const bool saved = m_configManager->save();
            if (!saved) {
                m_logManager->warning(QStringLiteral("Failed to persist locale preference"));
            }
        });

    if (loggingReady) {
        m_logManager->info(QStringLiteral("Application initialized"));
    }

    return configLoaded && loggingReady;
}

void Application::show()
{
    if (m_mainWindow) {
        m_mainWindow->show();
    }
}

void Application::shutdown()
{
    if (m_mainWindow) {
        m_mainWindow->saveSession();
    }
    if (m_configManager) {
        const bool saved = m_configManager->save();
        if (!saved && m_logManager) {
            m_logManager->warning(QStringLiteral("Failed to save configuration during shutdown"));
        }
    }
    if (m_logManager) {
        m_logManager->info(QStringLiteral("Application shutdown"));
    }
    if (m_crashRecoveryManager) {
        m_crashRecoveryManager->markAppExitedNormally();
    }
}

} // namespace pyraqt::app

#include "core/plugin/plugin_context.h"

#include "core/command/command_manager.h"
#include "core/config/config_manager.h"
#include "core/logging/log_manager.h"

namespace pyraqt::core {

PluginContext::PluginContext(CommandManager &commandManager, ConfigManager &configManager, LogManager &logManager)
    : m_commandManager(commandManager)
    , m_configManager(configManager)
    , m_logManager(logManager)
{
}

CommandManager &PluginContext::commandManager() const
{
    return m_commandManager;
}

ConfigManager &PluginContext::configManager() const
{
    return m_configManager;
}

LogManager &PluginContext::logManager() const
{
    return m_logManager;
}

} // namespace pyraqt::core

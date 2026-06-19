#pragma once

#include <QString>

namespace pyraqt::core {

class CommandManager;
class ConfigManager;
class LogManager;

class PluginContext final {
public:
    PluginContext(CommandManager &commandManager, ConfigManager &configManager, LogManager &logManager);

    [[nodiscard]] CommandManager &commandManager() const;
    [[nodiscard]] ConfigManager &configManager() const;
    [[nodiscard]] LogManager &logManager() const;

private:
    CommandManager &m_commandManager;
    ConfigManager &m_configManager;
    LogManager &m_logManager;
};

} // namespace pyraqt::core

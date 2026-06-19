#include "core/command/command_manager.h"

namespace pyraqt::core {

CommandManager::CommandManager(QObject *parent)
    : QObject(parent)
{
}

bool CommandManager::registerCommand(const CommandDescriptor &descriptor)
{
    for (const auto &command : m_commands) {
        if (command.id == descriptor.id) {
            return false;
        }
    }

    m_commands.push_back(descriptor);
    emit commandsChanged();
    return true;
}

void CommandManager::unregisterCommands(const QString &ownerId)
{
    QVector<CommandDescriptor> filtered;
    filtered.reserve(m_commands.size());
    bool changed = false;

    for (const auto &command : m_commands) {
        if (command.ownerId == ownerId) {
            changed = true;
            continue;
        }
        filtered.push_back(command);
    }

    if (changed) {
        m_commands = filtered;
        emit commandsChanged();
    }
}

QVector<CommandDescriptor> CommandManager::commands() const
{
    return m_commands;
}

bool CommandManager::execute(const QString &commandId) const
{
    for (const auto &command : m_commands) {
        if (command.id == commandId && command.enabled && static_cast<bool>(command.handler)) {
            command.handler();
            return true;
        }
    }
    return false;
}

} // namespace pyraqt::core

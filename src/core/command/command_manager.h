#pragma once

#include "core/command/command_descriptor.h"

#include <QObject>
#include <QVector>

namespace pyraqt::core {

class CommandManager final : public QObject {
    Q_OBJECT

public:
    explicit CommandManager(QObject *parent = nullptr);

    bool registerCommand(const CommandDescriptor &descriptor);
    void unregisterCommands(const QString &ownerId);
    [[nodiscard]] QVector<CommandDescriptor> commands() const;
    [[nodiscard]] bool execute(const QString &commandId) const;

signals:
    void commandsChanged();

private:
    QVector<CommandDescriptor> m_commands;
};

} // namespace pyraqt::core

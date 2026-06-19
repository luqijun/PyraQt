#pragma once

#include <QIcon>
#include <QString>
#include <QStringList>

#include <functional>

namespace pyraqt::core {

struct CommandDescriptor {
    QString id;
    QString ownerId;
    QString title;
    QString description;
    QString source;
    QStringList keywords;
    bool enabled = true;
    std::function<void()> handler;
};

} // namespace pyraqt::core

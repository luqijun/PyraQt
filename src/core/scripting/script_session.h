#pragma once

#include <QString>
#include <QStringList>

namespace pyraqt::core {

struct ScriptSession {
    QString scriptPath;
    QString workingDirectory;
    QStringList arguments;
    QString selectionCode;
    int timeoutMs = 15000;
    bool isSelection = false;
};

} // namespace pyraqt::core

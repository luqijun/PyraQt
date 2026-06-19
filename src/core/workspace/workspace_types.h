#pragma once

#include <QString>
#include <QStringList>

namespace pyraqt::core {

struct WorkspaceSession {
    QStringList openFiles;
    QString activeFile;
    QStringList recentFiles;
    QString fileBrowserRoot;
};

} // namespace pyraqt::core

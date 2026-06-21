#pragma once

#include <QString>
#include <QMap>
#include <QStringList>
#include <QVector>

namespace pyraqt::core {

struct PythonPluginCommand {
    QString id;
    QString title;
    QString description;
    QString script;
    QStringList arguments;
    QStringList keywords;
};

struct PluginInfo {
    QString id;
    QString name;
    QString version;
    QString description;
    QString type;
    QString entry;
    QString path;
    bool enabled = true;
    bool loaded = false;
    QString error;
    QString loadError;
    QString moduleName;
    QStringList dependencies;
    QMap<QString, QString> metadata;
    bool active = false;
    bool hasProcessingProvider = false;
    QVector<PythonPluginCommand> pythonCommands;
};

} // namespace pyraqt::core

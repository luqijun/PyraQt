#include "core/plugin/plugin_manager.h"

#include "core/command/command_manager.h"
#include "core/config/config_manager.h"
#include "core/logging/log_manager.h"
#include "core/plugin/iplugin.h"
#include "core/plugin/plugin_context.h"
#include "core/scripting/script_execution_manager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPluginLoader>

namespace pyraqt::core {
namespace {

QStringList pluginRootCandidates()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates;
    candidates << QDir(appDir).filePath(QStringLiteral("plugins"));
    candidates << QDir(appDir).filePath(QStringLiteral("../plugins"));
    candidates << QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../plugins"));
    return candidates;
}

} // namespace

PluginManager::PluginManager(
    CommandManager &commandManager,
    ConfigManager &configManager,
    LogManager &logManager,
    ScriptExecutionManager &scriptExecutionManager,
    QObject *parent)
    : QObject(parent)
    , m_commandManager(commandManager)
    , m_configManager(configManager)
    , m_logManager(logManager)
    , m_scriptExecutionManager(scriptExecutionManager)
    , m_pluginContext(new PluginContext(commandManager, configManager, logManager))
    , m_disabledPluginIds(m_configManager.value(QStringLiteral("plugins.disabled_ids")).toStringList())
{
}

PluginManager::~PluginManager()
{
    for (QPluginLoader *loader : m_pluginLoaders) {
        if (loader != nullptr) {
            loader->unload();
            delete loader;
        }
    }
    delete m_pluginContext;
}

void PluginManager::scanPlugins()
{
    m_plugins.clear();
    m_errors.clear();
    scanCppPlugins();
    scanPythonPlugins();
    emit pluginsRefreshed();
}

void PluginManager::loadEnabledPlugins()
{
    for (const auto &pluginInfo : m_plugins) {
        if (!pluginInfo.enabled) {
            continue;
        }

        if (pluginInfo.type == QStringLiteral("cpp")) {
            loadCppPlugin(pluginInfo);
        } else if (pluginInfo.type == QStringLiteral("python")) {
            loadPythonPlugin(pluginInfo);
        }
    }
    emit pluginsRefreshed();
}

bool PluginManager::setPluginEnabled(const QString &id, bool enabled)
{
    PluginInfo *plugin = findPlugin(id);
    if (plugin == nullptr) {
        return false;
    }

    plugin->enabled = enabled;
    if (enabled) {
        m_disabledPluginIds.removeAll(id);
    } else if (!m_disabledPluginIds.contains(id)) {
        m_disabledPluginIds.push_back(id);
        m_commandManager.unregisterCommands(id);
    }

    persistDisabledPlugins();
    emit pluginStateChanged(id, enabled);
    emit pluginsRefreshed();
    return true;
}

QVector<PluginInfo> PluginManager::plugins() const
{
    return m_plugins;
}

QStringList PluginManager::lastErrors() const
{
    return m_errors;
}

void PluginManager::scanCppPlugins()
{
    for (const QString &rootPath : pluginRootCandidates()) {
        const QString cppPath = QDir(rootPath).filePath(QStringLiteral("cpp"));
        QDir dir(cppPath);
        if (!dir.exists()) {
            continue;
        }

        const QFileInfoList entries = dir.entryInfoList(QDir::Files);
        for (const QFileInfo &entry : entries) {
            PluginInfo info;
            info.id = entry.baseName();
            info.name = entry.baseName();
            info.version = QStringLiteral("0.1.0");
            info.description = QStringLiteral("C++ plugin");
            info.type = QStringLiteral("cpp");
            info.path = entry.absoluteFilePath();
            info.enabled = isPluginEnabled(info.id, true);
            m_plugins.push_back(info);
        }
        return;
    }

    const QStringList configuredRoots = m_configManager.value(QStringLiteral("plugins.search_paths")).toStringList();
    for (const QString &rootPath : configuredRoots) {
        const QString cppPath = QDir(rootPath).filePath(QStringLiteral("cpp"));
        QDir dir(cppPath);
        if (!dir.exists()) {
            continue;
        }

        const QFileInfoList entries = dir.entryInfoList(QDir::Files);
        for (const QFileInfo &entry : entries) {
            PluginInfo info;
            info.id = entry.baseName();
            info.name = entry.baseName();
            info.version = QStringLiteral("0.1.0");
            info.description = QStringLiteral("C++ plugin");
            info.type = QStringLiteral("cpp");
            info.path = entry.absoluteFilePath();
            info.enabled = isPluginEnabled(info.id, true);
            m_plugins.push_back(info);
        }
        return;
    }
}

void PluginManager::scanPythonPlugins()
{
    for (const QString &rootPath : pluginRootCandidates()) {
        const QString pythonPath = QDir(rootPath).filePath(QStringLiteral("python"));
        QDir dir(pythonPath);
        if (!dir.exists()) {
            continue;
        }

        const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &entry : entries) {
            const QString manifestPath = QDir(entry.absoluteFilePath()).filePath(QStringLiteral("plugin.json"));
            QFile manifestFile(manifestPath);
            if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                m_errors.push_back(QStringLiteral("Failed to open plugin manifest: %1").arg(manifestPath));
                continue;
            }

            const QJsonDocument document = QJsonDocument::fromJson(manifestFile.readAll());
            if (!document.isObject()) {
                m_errors.push_back(QStringLiteral("Invalid plugin manifest: %1").arg(manifestPath));
                continue;
            }

            const QJsonObject root = document.object();
            PluginInfo info;
            info.id = root.value(QStringLiteral("id")).toString(entry.fileName());
            info.name = root.value(QStringLiteral("name")).toString(info.id);
            info.version = root.value(QStringLiteral("version")).toString(QStringLiteral("0.1.0"));
            info.description = root.value(QStringLiteral("description")).toString();
            info.type = QStringLiteral("python");
            info.entry = root.value(QStringLiteral("entry")).toString(QStringLiteral("__init__.py"));
            info.path = entry.absoluteFilePath();
            info.enabled = isPluginEnabled(info.id, root.value(QStringLiteral("enabledByDefault")).toBool(true));

            const QJsonArray commands = root.value(QStringLiteral("commands")).toArray();
            for (const auto &commandValue : commands) {
                const QJsonObject commandObject = commandValue.toObject();
                PythonPluginCommand command;
                command.id = commandObject.value(QStringLiteral("id")).toString();
                command.title = commandObject.value(QStringLiteral("title")).toString(command.id);
                command.description = commandObject.value(QStringLiteral("description")).toString();
                command.script = commandObject.value(QStringLiteral("script")).toString(info.entry);
                const QJsonArray argsArray = commandObject.value(QStringLiteral("arguments")).toArray();
                for (const auto &arg : argsArray) {
                    command.arguments.push_back(arg.toString());
                }
                const QJsonArray keywordsArray = commandObject.value(QStringLiteral("keywords")).toArray();
                for (const auto &keyword : keywordsArray) {
                    command.keywords.push_back(keyword.toString());
                }
                info.pythonCommands.push_back(command);
            }

            m_plugins.push_back(info);
        }
        return;
    }

    const QStringList configuredRoots = m_configManager.value(QStringLiteral("plugins.search_paths")).toStringList();
    for (const QString &rootPath : configuredRoots) {
        const QString pythonPath = QDir(rootPath).filePath(QStringLiteral("python"));
        QDir dir(pythonPath);
        if (!dir.exists()) {
            continue;
        }

        const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &entry : entries) {
            const QString manifestPath = QDir(entry.absoluteFilePath()).filePath(QStringLiteral("plugin.json"));
            QFile manifestFile(manifestPath);
            if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                m_errors.push_back(QStringLiteral("Failed to open plugin manifest: %1").arg(manifestPath));
                continue;
            }

            const QJsonDocument document = QJsonDocument::fromJson(manifestFile.readAll());
            if (!document.isObject()) {
                m_errors.push_back(QStringLiteral("Invalid plugin manifest: %1").arg(manifestPath));
                continue;
            }

            const QJsonObject root = document.object();
            PluginInfo info;
            info.id = root.value(QStringLiteral("id")).toString(entry.fileName());
            info.name = root.value(QStringLiteral("name")).toString(info.id);
            info.version = root.value(QStringLiteral("version")).toString(QStringLiteral("0.1.0"));
            info.description = root.value(QStringLiteral("description")).toString();
            info.type = QStringLiteral("python");
            info.entry = root.value(QStringLiteral("entry")).toString(QStringLiteral("__init__.py"));
            info.path = entry.absoluteFilePath();
            info.enabled = isPluginEnabled(info.id, root.value(QStringLiteral("enabledByDefault")).toBool(true));

            const QJsonArray commands = root.value(QStringLiteral("commands")).toArray();
            for (const auto &commandValue : commands) {
                const QJsonObject commandObject = commandValue.toObject();
                PythonPluginCommand command;
                command.id = commandObject.value(QStringLiteral("id")).toString();
                command.title = commandObject.value(QStringLiteral("title")).toString(command.id);
                command.description = commandObject.value(QStringLiteral("description")).toString();
                command.script = commandObject.value(QStringLiteral("script")).toString(info.entry);
                const QJsonArray argsArray = commandObject.value(QStringLiteral("arguments")).toArray();
                for (const auto &arg : argsArray) {
                    command.arguments.push_back(arg.toString());
                }
                const QJsonArray keywordsArray = commandObject.value(QStringLiteral("keywords")).toArray();
                for (const auto &keyword : keywordsArray) {
                    command.keywords.push_back(keyword.toString());
                }
                info.pythonCommands.push_back(command);
            }

            m_plugins.push_back(info);
        }
        return;
    }
}

void PluginManager::loadCppPlugin(const PluginInfo &pluginInfo)
{
    auto *loader = new QPluginLoader(pluginInfo.path, this);
    QObject *instance = loader->instance();
    if (instance == nullptr) {
        m_errors.push_back(loader->errorString());
        delete loader;
        return;
    }

    auto *plugin = qobject_cast<IPlugin *>(instance);
    if (plugin == nullptr) {
        m_errors.push_back(QStringLiteral("Plugin does not implement IPlugin: %1").arg(pluginInfo.path));
        loader->unload();
        delete loader;
        return;
    }

    if (!plugin->initialize(*m_pluginContext)) {
        m_errors.push_back(QStringLiteral("Failed to initialize plugin: %1").arg(pluginInfo.id));
        loader->unload();
        delete loader;
        return;
    }

    PluginInfo *stored = findPlugin(pluginInfo.id);
    if (stored != nullptr) {
        stored->loaded = true;
        stored->name = plugin->name();
        stored->version = plugin->version();
    }

    m_pluginLoaders.push_back(loader);
}

void PluginManager::loadPythonPlugin(const PluginInfo &pluginInfo)
{
    registerPythonPluginCommands(pluginInfo);
    PluginInfo *stored = findPlugin(pluginInfo.id);
    if (stored != nullptr) {
        stored->loaded = true;
    }
}

void PluginManager::registerPythonPluginCommands(const PluginInfo &pluginInfo)
{
    for (const auto &command : pluginInfo.pythonCommands) {
        CommandDescriptor descriptor;
        descriptor.id = command.id;
        descriptor.ownerId = pluginInfo.id;
        descriptor.title = command.title;
        descriptor.description = command.description;
        descriptor.source = pluginInfo.name;
        descriptor.keywords = command.keywords;
        descriptor.handler = [this, pluginInfo, command] {
            const QString scriptPath = QDir(pluginInfo.path).filePath(command.script);
            m_scriptExecutionManager.runFile(scriptPath, command.arguments);
        };
        m_commandManager.registerCommand(descriptor);
    }
}

bool PluginManager::isPluginEnabled(const QString &id, bool enabledByDefault) const
{
    if (m_disabledPluginIds.contains(id)) {
        return false;
    }
    return enabledByDefault;
}

void PluginManager::persistDisabledPlugins()
{
    m_configManager.setValue(QStringLiteral("plugins.disabled_ids"), m_disabledPluginIds);
    const bool saved = m_configManager.save();
    if (!saved) {
        m_logManager.warning(QStringLiteral("Failed to persist disabled plugin list"));
    }
}

QStringList PluginManager::pluginSearchPaths() const
{
    QStringList searchPaths;
    for (const QString &rootPath : pluginRootCandidates()) {
        searchPaths.push_back(QDir(rootPath).filePath(QStringLiteral("cpp")));
        searchPaths.push_back(QDir(rootPath).filePath(QStringLiteral("python")));
    }
    return searchPaths;
}

PluginInfo *PluginManager::findPlugin(const QString &id)
{
    for (auto &plugin : m_plugins) {
        if (plugin.id == id) {
            return &plugin;
        }
    }
    return nullptr;
}

} // namespace pyraqt::core

#include "core/plugin/plugin_manager.h"

#include "core/command/command_manager.h"
#include "core/config/config_manager.h"
#include "core/logging/log_manager.h"
#include "core/plugin/iplugin.h"
#include "core/plugin/plugin_context.h"
#include "core/scripting/python_runtime_manager.h"
#include "core/scripting/script_execution_manager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPluginLoader>
#include <QRegularExpression>
#include <QSettings>

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
    PythonRuntimeManager &pythonRuntimeManager,
    QObject *parent)
    : QObject(parent)
    , m_commandManager(commandManager)
    , m_configManager(configManager)
    , m_logManager(logManager)
    , m_scriptExecutionManager(scriptExecutionManager)
    , m_pythonRuntimeManager(pythonRuntimeManager)
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
        if (plugin->type == QStringLiteral("python") && !plugin->loaded) {
            loadPythonPlugin(*plugin);
        }
    } else if (!m_disabledPluginIds.contains(id)) {
        if (plugin->type == QStringLiteral("python") && plugin->loaded) {
            unloadPythonPlugin(*plugin);
        }
        m_disabledPluginIds.push_back(id);
        m_commandManager.unregisterCommands(id);
    }

    persistDisabledPlugins();
    emit pluginStateChanged(id, enabled);
    emit pluginsRefreshed();
    return true;
}

bool PluginManager::reloadPlugin(const QString &id)
{
    PluginInfo *plugin = findPlugin(id);
    if (plugin == nullptr || !plugin->enabled) {
        return false;
    }

    m_commandManager.unregisterCommands(id);
    if (plugin->type == QStringLiteral("python")) {
        if (plugin->loaded) {
            unloadPythonPlugin(*plugin);
        }
        QString moduleName = plugin->moduleName.isEmpty() ? QFileInfo(plugin->path).fileName() : plugin->moduleName;
        moduleName.replace('\'', QStringLiteral("\\'"));
        const ScriptResult reloadResult = m_pythonRuntimeManager.runString(
            QStringLiteral("import sys\nsys.modules.pop('%1', None)\n").arg(moduleName),
            false);
        Q_UNUSED(reloadResult)
        loadPythonPlugin(*plugin);
        emit pluginsRefreshed();
        return true;
    }

    emit pluginsRefreshed();
    return false;
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
            PluginInfo info = readPythonPluginInfo(entry);
            if (info.id.isEmpty()) {
                m_errors.push_back(QStringLiteral("Invalid Python plugin: %1").arg(entry.absoluteFilePath()));
                continue;
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
            PluginInfo info = readPythonPluginInfo(entry);
            if (info.id.isEmpty()) {
                m_errors.push_back(QStringLiteral("Invalid Python plugin: %1").arg(entry.absoluteFilePath()));
                continue;
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
    m_pythonRuntimeManager.initializeEmbedded({}, pluginSearchPaths());
    QString moduleName = pluginInfo.moduleName.isEmpty() ? QFileInfo(pluginInfo.path).fileName() : pluginInfo.moduleName;
    QString escapedPath = pluginInfo.path;
    escapedPath.replace('\\', QStringLiteral("\\\\"));
    escapedPath.replace('\'', QStringLiteral("\\'"));
    moduleName.replace('\'', QStringLiteral("\\'"));

    const QString code = QStringLiteral(
                             "import importlib, sys\n"
                             "plugin_path = '%1'\n"
                             "if plugin_path not in sys.path:\n"
                             "    sys.path.insert(0, plugin_path)\n"
                             "_pyraqt_plugin_module = importlib.import_module('%2')\n"
                             "_pyraqt_plugin_instance = None\n"
                             "if hasattr(_pyraqt_plugin_module, 'classFactory'):\n"
                             "    _pyraqt_plugin_instance = _pyraqt_plugin_module.classFactory(iface)\n"
                             "    setattr(_pyraqt_plugin_module, '_pyraqt_plugin_instance', _pyraqt_plugin_instance)\n"
                             "    if hasattr(_pyraqt_plugin_instance, 'initGui'):\n"
                             "        _pyraqt_plugin_instance.initGui()\n")
                             .arg(escapedPath, moduleName);
    const ScriptResult result = m_pythonRuntimeManager.runString(code, false);
    if (!result.success) {
        PluginInfo *stored = findPlugin(pluginInfo.id);
        if (stored != nullptr) {
            stored->loadError = result.traceback;
            stored->error = result.traceback;
        }
        m_errors.push_back(QStringLiteral("Failed to load Python plugin %1: %2").arg(pluginInfo.id, result.traceback));
        return;
    }

    registerPythonPluginCommands(pluginInfo);
    PluginInfo *stored = findPlugin(pluginInfo.id);
    if (stored != nullptr) {
        stored->loaded = true;
        stored->active = true;
    }
}

void PluginManager::unloadPythonPlugin(const PluginInfo &pluginInfo)
{
    QString moduleName = pluginInfo.moduleName.isEmpty() ? QFileInfo(pluginInfo.path).fileName() : pluginInfo.moduleName;
    moduleName.replace('\'', QStringLiteral("\\'"));
    const QString code = QStringLiteral(
                             "import sys\n"
                             "_pyraqt_plugin_module = sys.modules.get('%1')\n"
                             "if _pyraqt_plugin_module is not None:\n"
                             "    _pyraqt_plugin_instance = getattr(_pyraqt_plugin_module, '_pyraqt_plugin_instance', None)\n"
                             "    if _pyraqt_plugin_instance is not None and hasattr(_pyraqt_plugin_instance, 'unload'):\n"
                             "        _pyraqt_plugin_instance.unload()\n"
                             "    if hasattr(_pyraqt_plugin_module, '_pyraqt_plugin_instance'):\n"
                             "        delattr(_pyraqt_plugin_module, '_pyraqt_plugin_instance')\n")
                             .arg(moduleName);
    const ScriptResult result = m_pythonRuntimeManager.runString(code, false);
    PluginInfo *stored = findPlugin(pluginInfo.id);
    if (stored != nullptr) {
        stored->loaded = false;
        stored->active = false;
        stored->error = result.success ? QString() : result.traceback;
        stored->loadError = stored->error;
    }
    if (!result.success) {
        m_errors.push_back(QStringLiteral("Failed to unload Python plugin %1: %2").arg(pluginInfo.id, result.traceback));
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

PluginInfo PluginManager::readPythonPluginInfo(const QFileInfo &entry) const
{
    PluginInfo info;
    info.id = entry.fileName();
    info.name = info.id;
    info.version = QStringLiteral("0.1.0");
    info.type = QStringLiteral("python");
    info.entry = QStringLiteral("__init__.py");
    info.path = entry.absoluteFilePath();
    info.moduleName = entry.fileName();

    const QString jsonPath = QDir(entry.absoluteFilePath()).filePath(QStringLiteral("plugin.json"));
    QFile jsonFile(jsonPath);
    bool enabledByDefault = true;
    if (jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QJsonDocument document = QJsonDocument::fromJson(jsonFile.readAll());
        if (document.isObject()) {
            const QJsonObject root = document.object();
            info.id = root.value(QStringLiteral("id")).toString(info.id);
            info.name = root.value(QStringLiteral("name")).toString(info.id);
            info.version = root.value(QStringLiteral("version")).toString(info.version);
            info.description = root.value(QStringLiteral("description")).toString();
            info.entry = root.value(QStringLiteral("entry")).toString(info.entry);
            info.moduleName = root.value(QStringLiteral("module")).toString(entry.fileName());
            enabledByDefault = root.value(QStringLiteral("enabledByDefault")).toBool(true);
            info.hasProcessingProvider = root.value(QStringLiteral("hasProcessingProvider")).toBool(false);

            const QJsonArray dependencies = root.value(QStringLiteral("dependencies")).toArray();
            for (const auto &dependency : dependencies) {
                info.dependencies.push_back(dependency.toString());
            }

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
        }
    }

    const QString metadataPath = QDir(entry.absoluteFilePath()).filePath(QStringLiteral("metadata.txt"));
    QFile metadataFile(metadataPath);
    if (metadataFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!metadataFile.atEnd()) {
            const QString line = QString::fromUtf8(metadataFile.readLine()).trimmed();
            if (line.isEmpty() || line.startsWith(QLatin1Char('[')) || line.startsWith(QLatin1Char('#')) || !line.contains(QLatin1Char('='))) {
                continue;
            }
            const int separator = line.indexOf(QLatin1Char('='));
            const QString key = line.left(separator).trimmed();
            const QString value = line.mid(separator + 1).trimmed();
            info.metadata.insert(key, value);
        }
        info.name = info.metadata.value(QStringLiteral("name"), info.name);
        info.version = info.metadata.value(QStringLiteral("version"), info.version);
        info.description = info.metadata.value(QStringLiteral("description"), info.description);
        info.hasProcessingProvider = info.hasProcessingProvider
            || info.metadata.value(QStringLiteral("hasProcessingProvider")).compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0
            || info.metadata.value(QStringLiteral("hasProcessingProvider")).compare(QStringLiteral("yes"), Qt::CaseInsensitive) == 0;
        const QString dependencies = info.metadata.value(QStringLiteral("plugin_dependencies"));
        if (!dependencies.isEmpty()) {
            info.dependencies.append(dependencies.split(QRegularExpression(QStringLiteral("\\s*,\\s*")), Qt::SkipEmptyParts));
        }
    }

    if (!QFileInfo::exists(QDir(entry.absoluteFilePath()).filePath(info.entry))) {
        return {};
    }

    info.enabled = isPluginEnabled(info.id, enabledByDefault);
    return info;
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

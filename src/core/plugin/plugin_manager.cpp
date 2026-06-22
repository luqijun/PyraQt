#include "core/plugin/plugin_manager.h"

#include "core/command/command_manager.h"
#include "core/config/config_manager.h"
#include "core/logging/log_manager.h"
#include "core/plugin/iplugin.h"
#include "core/plugin/plugin_context.h"
#include "core/scripting/python/python_feature_manager.h"
#include "core/scripting/python/python_runtime_manager.h"
#include "core/scripting/python/script_execution_manager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPluginLoader>
#include <QRegularExpression>
#include <QSet>

namespace pyraqt::core {
namespace {

QString sanitizeModuleFragment(QString value)
{
    value.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_]")), QStringLiteral("_"));
    if (value.isEmpty()) {
        return QStringLiteral("plugin");
    }
    if (value.at(0).isDigit()) {
        value.prepend(QLatin1Char('_'));
    }
    return value;
}

} // namespace

PluginManager::PluginManager(
    CommandManager &commandManager,
    ConfigManager &configManager,
    LogManager &logManager,
    ScriptExecutionManager &scriptExecutionManager,
    PythonFeatureManager &pythonFeatureManager,
    PythonRuntimeManager &pythonRuntimeManager,
    QObject *parent)
    : QObject(parent)
    , m_commandManager(commandManager)
    , m_configManager(configManager)
    , m_logManager(logManager)
    , m_scriptExecutionManager(scriptExecutionManager)
    , m_pythonFeatureManager(pythonFeatureManager)
    , m_pythonRuntimeManager(pythonRuntimeManager)
    , m_pluginContext(new PluginContext(commandManager, configManager, logManager))
    , m_disabledPluginIds(m_configManager.value(QStringLiteral("plugins.disabled_ids")).toStringList())
{
}

PluginManager::~PluginManager()
{
    unloadAllPlugins();
    delete m_pluginContext;
}

void PluginManager::scanPlugins()
{
    unloadAllPlugins();
    m_plugins.clear();
    m_errors.clear();
    scanCppPlugins();
    scanPythonPlugins();
    emit pluginsRefreshed();
}

void PluginManager::loadEnabledPlugins()
{
    for (auto &pluginInfo : m_plugins) {
        if (!pluginInfo.enabled) {
            continue;
        }
        loadPlugin(pluginInfo);
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
        if (!plugin->loaded) {
            loadPlugin(*plugin);
        }
    } else {
        if (!m_disabledPluginIds.contains(id)) {
            m_disabledPluginIds.push_back(id);
        }
        if (plugin->loaded) {
            if (plugin->type == QStringLiteral("cpp")) {
                unloadCppPlugin(*plugin);
            } else if (plugin->type == QStringLiteral("python")) {
                unloadPythonPlugin(*plugin);
            }
        }
        m_commandManager.unregisterCommands(id);
        m_commandManager.unregisterCommands(plugin->metadata.value(QStringLiteral("runtimeId")));
        m_pythonFeatureManager.unregisterExpressionFunctions(id);
        m_pythonFeatureManager.unregisterProcessingAlgorithms(id);
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

    bool unloaded = true;
    if (plugin->loaded) {
        unloaded = plugin->type == QStringLiteral("cpp")
            ? unloadCppPlugin(*plugin)
            : unloadPythonPlugin(*plugin);
    }
    if (!unloaded) {
        emit pluginsRefreshed();
        return false;
    }

    const bool loaded = loadPlugin(*plugin);
    emit pluginsRefreshed();
    return loaded;
}

QVector<PluginInfo> PluginManager::plugins() const
{
    return m_plugins;
}

QStringList PluginManager::lastErrors() const
{
    return m_errors;
}

void PluginManager::unloadAllPlugins()
{
    for (int index = m_plugins.size() - 1; index >= 0; --index) {
        PluginInfo &plugin = m_plugins[index];
        if (!plugin.loaded) {
            continue;
        }
        if (plugin.type == QStringLiteral("cpp")) {
            unloadCppPlugin(plugin);
        } else if (plugin.type == QStringLiteral("python")) {
            unloadPythonPlugin(plugin);
        }
    }
}

void PluginManager::scanCppPlugins()
{
    QSet<QString> discoveredIds;
    for (const QString &rootPath : pluginRootPaths()) {
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
            if (discoveredIds.contains(info.id) || findPlugin(info.id) != nullptr) {
                m_errors.push_back(QStringLiteral("Duplicate plugin id skipped: %1").arg(info.id));
                continue;
            }
            discoveredIds.insert(info.id);
            m_plugins.push_back(info);
        }
    }
}

void PluginManager::scanPythonPlugins()
{
    QSet<QString> discoveredIds;
    for (const QString &rootPath : pluginRootPaths()) {
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
            if (discoveredIds.contains(info.id) || findPlugin(info.id) != nullptr) {
                m_errors.push_back(QStringLiteral("Duplicate plugin id skipped: %1").arg(info.id));
                continue;
            }
            discoveredIds.insert(info.id);
            m_plugins.push_back(info);
        }
    }
}

bool PluginManager::loadPlugin(PluginInfo &pluginInfo)
{
    if (!pluginInfo.enabled || pluginInfo.loaded) {
        return pluginInfo.loaded;
    }

    QString dependencyReason;
    if (!dependenciesSatisfied(pluginInfo, &dependencyReason)) {
        appendPluginError(pluginInfo, dependencyReason);
        return false;
    }

    for (const QString &dependencyId : pluginInfo.dependencies) {
        PluginInfo *dependency = findPlugin(dependencyId);
        if (dependency == nullptr || dependency->loaded || !dependency->enabled) {
            continue;
        }
        if (dependency->metadata.value(QStringLiteral("_pyraqt_load_state")) == QStringLiteral("loading")) {
            appendPluginError(pluginInfo, QStringLiteral("Circular dependency detected with %1").arg(dependencyId));
            return false;
        }
        dependency->metadata.insert(QStringLiteral("_pyraqt_load_state"), QStringLiteral("loading"));
        const bool dependencyLoaded = loadPlugin(*dependency);
        dependency->metadata.remove(QStringLiteral("_pyraqt_load_state"));
        if (!dependencyLoaded) {
            appendPluginError(pluginInfo, QStringLiteral("Dependency failed to load: %1").arg(dependencyId));
            return false;
        }
    }

    clearPluginError(pluginInfo);
    if (pluginInfo.type == QStringLiteral("cpp")) {
        return loadCppPlugin(pluginInfo);
    }
    if (pluginInfo.type == QStringLiteral("python")) {
        return loadPythonPlugin(pluginInfo);
    }

    appendPluginError(pluginInfo, QStringLiteral("Unknown plugin type: %1").arg(pluginInfo.type));
    return false;
}

bool PluginManager::loadCppPlugin(PluginInfo &pluginInfo)
{
    auto *loader = new QPluginLoader(pluginInfo.path, this);
    QObject *instance = loader->instance();
    if (instance == nullptr) {
        appendPluginError(pluginInfo, loader->errorString());
        delete loader;
        return false;
    }

    auto *plugin = qobject_cast<IPlugin *>(instance);
    if (plugin == nullptr) {
        appendPluginError(pluginInfo, QStringLiteral("Plugin does not implement IPlugin: %1").arg(pluginInfo.path));
        loader->unload();
        delete loader;
        return false;
    }

    if (!plugin->initialize(*m_pluginContext)) {
        appendPluginError(pluginInfo, QStringLiteral("Failed to initialize plugin: %1").arg(pluginInfo.id));
        loader->unload();
        delete loader;
        return false;
    }

    pluginInfo.loaded = true;
    pluginInfo.active = true;
    pluginInfo.name = plugin->name();
    pluginInfo.version = plugin->version();
    pluginInfo.metadata.insert(QStringLiteral("runtimeId"), plugin->id());
    clearPluginError(pluginInfo);
    m_pluginLoaders.insert(pluginInfo.id, loader);
    return true;
}

bool PluginManager::unloadCppPlugin(PluginInfo &pluginInfo)
{
    QPluginLoader *loader = m_pluginLoaders.take(pluginInfo.id);
    if (loader == nullptr) {
        pluginInfo.loaded = false;
        pluginInfo.active = false;
        return true;
    }

    bool ok = true;
    QObject *instance = loader->instance();
    if (auto *plugin = qobject_cast<IPlugin *>(instance); plugin != nullptr) {
        plugin->shutdown();
        m_commandManager.unregisterCommands(plugin->id());
    }

    m_commandManager.unregisterCommands(pluginInfo.id);
    m_commandManager.unregisterCommands(pluginInfo.metadata.value(QStringLiteral("runtimeId")));

    if (!loader->unload()) {
        ok = false;
        appendPluginError(pluginInfo, loader->errorString());
    } else {
        clearPluginError(pluginInfo);
    }

    pluginInfo.loaded = false;
    pluginInfo.active = false;
    delete loader;
    return ok;
}

bool PluginManager::loadPythonPlugin(PluginInfo &pluginInfo)
{
    if (!m_pythonRuntimeManager.initializeEmbedded({}, pluginSearchPaths())) {
        appendPluginError(pluginInfo, QStringLiteral("Failed to initialize embedded Python runtime"));
        return false;
    }

    const QString entryPath = QDir(pluginInfo.path).filePath(pluginInfo.entry);
    QString escapedPluginPath = QDir::cleanPath(pluginInfo.path);
    escapedPluginPath.replace('\\', QStringLiteral("\\\\"));
    escapedPluginPath.replace('\'', QStringLiteral("\\'"));

    QString escapedEntryPath = QDir::cleanPath(entryPath);
    escapedEntryPath.replace('\\', QStringLiteral("\\\\"));
    escapedEntryPath.replace('\'', QStringLiteral("\\'"));

    const QString runtimeModuleName = pythonRootModuleName(pluginInfo);
    pluginInfo.runtimeModuleName = runtimeModuleName;

    QString escapedRuntimeModule = runtimeModuleName;
    escapedRuntimeModule.replace('\'', QStringLiteral("\\'"));

    const bool isPackageEntry = QFileInfo(entryPath).fileName() == QStringLiteral("__init__.py");
    const QString code = QStringLiteral(
                             "import importlib.util, sys\n"
                             "plugin_path = '%1'\n"
                             "entry_path = '%2'\n"
                             "runtime_name = '%3'\n"
                             "if plugin_path not in sys.path:\n"
                             "    sys.path.insert(0, plugin_path)\n"
                             "spec = importlib.util.spec_from_file_location(runtime_name, entry_path, submodule_search_locations=%4)\n"
                             "if spec is None or spec.loader is None:\n"
                             "    raise ImportError(f'Unable to create plugin spec for {entry_path}')\n"
                             "module = importlib.util.module_from_spec(spec)\n"
                             "sys.modules[runtime_name] = module\n"
                             "spec.loader.exec_module(module)\n"
                             "plugin_instance = None\n"
                             "if hasattr(module, 'classFactory'):\n"
                             "    plugin_instance = module.classFactory(iface)\n"
                             "    setattr(module, '_pyraqt_plugin_instance', plugin_instance)\n"
                             "    if hasattr(plugin_instance, 'initGui'):\n"
                             "        plugin_instance.initGui()\n")
                             .arg(
                                 escapedPluginPath,
                                 escapedEntryPath,
                                 escapedRuntimeModule,
                                 isPackageEntry ? QStringLiteral("[plugin_path]") : QStringLiteral("None"));

    m_pythonRuntimeManager.setRegistrationOwnerId(pluginInfo.id);
    const ScriptResult result = m_pythonRuntimeManager.runString(code, false);
    m_pythonRuntimeManager.setRegistrationOwnerId(QString());
    if (!result.success) {
        const QString cleanupCode = QStringLiteral(
                                        "import sys\n"
                                        "for name in list(sys.modules):\n"
                                        "    if name == '%1' or name.startswith('%1.'):\n"
                                        "        sys.modules.pop(name, None)\n")
                                        .arg(escapedRuntimeModule);
        const ScriptResult cleanupResult = m_pythonRuntimeManager.runString(cleanupCode, false);
        Q_UNUSED(cleanupResult)
        appendPluginError(pluginInfo, QStringLiteral("Failed to load Python plugin %1: %2").arg(pluginInfo.id, result.traceback));
        return false;
    }

    registerPythonPluginCommands(pluginInfo);
    pluginInfo.loaded = true;
    pluginInfo.active = true;
    clearPluginError(pluginInfo);
    return true;
}

bool PluginManager::unloadPythonPlugin(PluginInfo &pluginInfo)
{
    const QString runtimeModuleName = pluginInfo.runtimeModuleName.isEmpty()
        ? pythonRootModuleName(pluginInfo)
        : pluginInfo.runtimeModuleName;
    QString escapedRuntimeModule = runtimeModuleName;
    escapedRuntimeModule.replace('\'', QStringLiteral("\\'"));

    const QString code = QStringLiteral(
                             "import sys\n"
                             "module = sys.modules.get('%1')\n"
                             "if module is not None:\n"
                             "    plugin_instance = getattr(module, '_pyraqt_plugin_instance', None)\n"
                             "    if plugin_instance is not None and hasattr(plugin_instance, 'unload'):\n"
                             "        plugin_instance.unload()\n"
                             "for name in list(sys.modules):\n"
                             "    if name == '%1' or name.startswith('%1.'):\n"
                             "        sys.modules.pop(name, None)\n")
                             .arg(escapedRuntimeModule);

    m_commandManager.unregisterCommands(pluginInfo.id);
    m_pythonFeatureManager.unregisterExpressionFunctions(pluginInfo.id);
    m_pythonFeatureManager.unregisterProcessingAlgorithms(pluginInfo.id);

    m_pythonRuntimeManager.setRegistrationOwnerId(pluginInfo.id);
    const ScriptResult result = m_pythonRuntimeManager.runString(code, false);
    m_pythonRuntimeManager.setRegistrationOwnerId(QString());

    pluginInfo.loaded = false;
    pluginInfo.active = false;
    if (!result.success) {
        appendPluginError(pluginInfo, QStringLiteral("Failed to unload Python plugin %1: %2").arg(pluginInfo.id, result.traceback));
        return false;
    }

    clearPluginError(pluginInfo);
    return true;
}

void PluginManager::registerPythonPluginCommands(const PluginInfo &pluginInfo)
{
    for (const auto &command : pluginInfo.pythonCommands) {
        if (command.id.trimmed().isEmpty()) {
            continue;
        }

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
    info.runtimeModuleName = pythonRootModuleName(info);

    const QString jsonPath = QDir(entry.absoluteFilePath()).filePath(QStringLiteral("plugin.json"));
    QFile jsonFile(jsonPath);
    bool enabledByDefault = true;
    if (jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
        if (parseError.error == QJsonParseError::NoError && document.isObject()) {
            const QJsonObject root = document.object();
            info.id = root.value(QStringLiteral("id")).toString(info.id).trimmed();
            info.name = root.value(QStringLiteral("name")).toString(info.id).trimmed();
            info.version = root.value(QStringLiteral("version")).toString(info.version).trimmed();
            info.description = root.value(QStringLiteral("description")).toString().trimmed();
            info.entry = root.value(QStringLiteral("entry")).toString(info.entry).trimmed();
            info.moduleName = root.value(QStringLiteral("module")).toString(entry.fileName()).trimmed();
            enabledByDefault = root.value(QStringLiteral("enabledByDefault")).toBool(true);
            info.hasProcessingProvider = root.value(QStringLiteral("hasProcessingProvider")).toBool(false);

            const QJsonArray dependencies = root.value(QStringLiteral("dependencies")).toArray();
            for (const auto &dependency : dependencies) {
                const QString dependencyId = dependency.toString().trimmed();
                if (!dependencyId.isEmpty()) {
                    info.dependencies.push_back(dependencyId);
                }
            }

            const QJsonArray commands = root.value(QStringLiteral("commands")).toArray();
            for (const auto &commandValue : commands) {
                const QJsonObject commandObject = commandValue.toObject();
                PythonPluginCommand command;
                command.id = commandObject.value(QStringLiteral("id")).toString().trimmed();
                command.title = commandObject.value(QStringLiteral("title")).toString(command.id).trimmed();
                command.description = commandObject.value(QStringLiteral("description")).toString().trimmed();
                command.script = commandObject.value(QStringLiteral("script")).toString(info.entry).trimmed();
                const QJsonArray argsArray = commandObject.value(QStringLiteral("arguments")).toArray();
                for (const auto &arg : argsArray) {
                    command.arguments.push_back(arg.toString());
                }
                const QJsonArray keywordsArray = commandObject.value(QStringLiteral("keywords")).toArray();
                for (const auto &keyword : keywordsArray) {
                    command.keywords.push_back(keyword.toString());
                }
                if (!command.id.isEmpty()) {
                    info.pythonCommands.push_back(command);
                }
            }
        } else {
            info.error = QStringLiteral("Invalid plugin.json: %1").arg(parseError.errorString());
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

    info.id = info.id.trimmed();
    info.name = info.name.trimmed();
    info.entry = info.entry.trimmed().isEmpty() ? QStringLiteral("__init__.py") : info.entry.trimmed();
    info.runtimeModuleName = pythonRootModuleName(info);

    const QString entryPath = QDir(entry.absoluteFilePath()).filePath(info.entry);
    if (info.id.isEmpty() || info.name.isEmpty() || !QFileInfo::exists(entryPath)) {
        return {};
    }

    for (auto &command : info.pythonCommands) {
        if (command.script.isEmpty()) {
            command.script = info.entry;
        }
    }

    info.dependencies.removeDuplicates();
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
    m_disabledPluginIds.removeDuplicates();
    m_configManager.setValue(QStringLiteral("plugins.disabled_ids"), m_disabledPluginIds);
    const bool saved = m_configManager.save();
    if (!saved) {
        m_logManager.warning(QStringLiteral("Failed to persist disabled plugin list"));
    }
}

QStringList PluginManager::pluginSearchPaths() const
{
    QStringList searchPaths;
    for (const QString &rootPath : pluginRootPaths()) {
        searchPaths.push_back(QDir(rootPath).filePath(QStringLiteral("cpp")));
        searchPaths.push_back(QDir(rootPath).filePath(QStringLiteral("python")));
    }
    searchPaths.removeDuplicates();
    return searchPaths;
}

QStringList PluginManager::pluginRootPaths() const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList roots;
    roots << QDir(appDir).filePath(QStringLiteral("plugins"));
    roots << QDir(appDir).filePath(QStringLiteral("../plugins"));
    roots << QDir(appDir).filePath(QStringLiteral("../../plugins"));
    roots << QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../plugins"));
    roots << m_configManager.value(QStringLiteral("plugins.search_paths")).toStringList();

    QStringList cleaned;
    for (const QString &root : roots) {
        if (root.trimmed().isEmpty()) {
            continue;
        }
        cleaned.push_back(QDir::cleanPath(QFileInfo(root).absoluteFilePath()));
    }
    cleaned.removeDuplicates();
    return cleaned;
}

QString PluginManager::pythonEntryModuleName(const PluginInfo &pluginInfo) const
{
    const QFileInfo entryInfo(pluginInfo.entry);
    return entryInfo.completeBaseName().isEmpty()
        ? sanitizeModuleFragment(pluginInfo.id)
        : sanitizeModuleFragment(entryInfo.completeBaseName());
}

QString PluginManager::pythonRootModuleName(const PluginInfo &pluginInfo) const
{
    return QStringLiteral("pyraqt_plugin_%1_%2")
        .arg(sanitizeModuleFragment(pluginInfo.id), pythonEntryModuleName(pluginInfo));
}

bool PluginManager::dependenciesSatisfied(const PluginInfo &pluginInfo, QString *reason) const
{
    for (const QString &dependencyId : pluginInfo.dependencies) {
        const PluginInfo *dependency = findPlugin(dependencyId);
        if (dependency == nullptr) {
            if (reason != nullptr) {
                *reason = QStringLiteral("Missing dependency: %1").arg(dependencyId);
            }
            return false;
        }
        if (!dependency->enabled) {
            if (reason != nullptr) {
                *reason = QStringLiteral("Dependency is disabled: %1").arg(dependencyId);
            }
            return false;
        }
    }
    return true;
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

const PluginInfo *PluginManager::findPlugin(const QString &id) const
{
    for (const auto &plugin : m_plugins) {
        if (plugin.id == id) {
            return &plugin;
        }
    }
    return nullptr;
}

void PluginManager::appendPluginError(PluginInfo &pluginInfo, const QString &message)
{
    pluginInfo.error = message;
    pluginInfo.loadError = message;
    pluginInfo.loaded = false;
    pluginInfo.active = false;
    m_errors.push_back(message);
}

void PluginManager::clearPluginError(PluginInfo &pluginInfo)
{
    pluginInfo.error.clear();
    pluginInfo.loadError.clear();
}

} // namespace pyraqt::core

#include "core/config/config_manager.h"

#include <QDir>
#include <QFileDevice>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QProcessEnvironment>

namespace pyraqt::core {
namespace {

QString dataRootPath()
{
    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (environment.contains(QStringLiteral("PYRAQT_DATA_DIR"))) {
        return environment.value(QStringLiteral("PYRAQT_DATA_DIR"));
    }
    return QDir::temp().filePath(QStringLiteral("PyraQt"));
}

QString ensureConfigPath()
{
    const QString basePath = QDir(dataRootPath()).filePath(QStringLiteral("config"));
    QDir dir(basePath);
    dir.mkpath(QStringLiteral("."));
    return dir.filePath(QStringLiteral("config.json"));
}

} // namespace

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_settings(QStringLiteral("PyraQt"), QStringLiteral("PyraQt"))
    , m_values(defaults())
{
}

QVariant ConfigManager::value(const QString &key, const QVariant &fallback) const
{
    if (m_values.contains(key)) {
        return m_values.value(key).toVariant();
    }
    return m_settings.value(key, fallback);
}

void ConfigManager::setValue(const QString &key, const QVariant &value)
{
    m_values.insert(key, QJsonValue::fromVariant(value));
    m_settings.setValue(key, value);
    emit valueChanged(key, value);
}

bool ConfigManager::load()
{
    QFile file(configFilePath());
    if (!file.exists()) {
        return save();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return false;
    }

    m_values = defaults();
    const QJsonObject loaded = document.object();
    for (auto it = loaded.begin(); it != loaded.end(); ++it) {
        m_values.insert(it.key(), it.value());
    }

    return true;
}

bool ConfigManager::save() const
{
    QFile file(configFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(m_values).toJson(QJsonDocument::Indented));
    return true;
}

QString ConfigManager::configFilePath() const
{
    return ensureConfigPath();
}

QJsonObject ConfigManager::defaults() const
{
    QJsonObject object;
    object.insert(QStringLiteral("theme"), QStringLiteral("dark"));
    object.insert(QStringLiteral("locale"), QStringLiteral("zh_CN"));
    object.insert(QStringLiteral("python.interpreter_path"), QStringLiteral("python3"));
    object.insert(QStringLiteral("python.execution_timeout_ms"), 15000);
    object.insert(QStringLiteral("python.code_completion_enabled"), true);
    object.insert(QStringLiteral("python.completion_trigger_threshold"), 2);
    object.insert(QStringLiteral("python.last_script_path"), QString());
    object.insert(QStringLiteral("python.last_directory"), QString());
    object.insert(QStringLiteral("plugins.disabled_ids"), QJsonArray());
    object.insert(QStringLiteral("plugins.search_paths"), QJsonArray());
    object.insert(QStringLiteral("command_palette.last_query"), QString());
    object.insert(QStringLiteral("updates.auto_check"), false);
    object.insert(QStringLiteral("updates.channel"), QStringLiteral("stable"));
    object.insert(QStringLiteral("updates.feed_url"), QString());
    object.insert(QStringLiteral("updates.last_check"), QString());
    object.insert(QStringLiteral("runtime.last_exit_clean"), true);
    object.insert(QStringLiteral("runtime.last_start"), QString());
    object.insert(QStringLiteral("runtime.last_exit"), QString());
    object.insert(QStringLiteral("workspace.recent_files"), QJsonArray());
    object.insert(QStringLiteral("workspace.open_files"), QJsonArray());
    object.insert(QStringLiteral("workspace.active_file"), QString());
    object.insert(QStringLiteral("workspace.restore_last_session"), true);
    object.insert(QStringLiteral("workspace.file_browser_root"), QString());
    object.insert(QStringLiteral("workspace.max_recent_files"), 10);
    object.insert(QStringLiteral("window.restore_layout"), true);
    return object;
}

} // namespace pyraqt::core

#include "core/logging/log_manager.h"

#include <QDateTime>
#include <QDir>
#include <QFileDevice>
#include <QFile>
#include <QProcessEnvironment>
#include <QTextStream>
#include <QtGlobal>

#if PYRAQT_HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

namespace pyraqt::core {
namespace {

LogManager *g_logManager = nullptr;

QString dataRootPath()
{
    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (environment.contains(QStringLiteral("PYRAQT_DATA_DIR"))) {
        return environment.value(QStringLiteral("PYRAQT_DATA_DIR"));
    }
    return QDir::temp().filePath(QStringLiteral("PyraQt"));
}

QString logPath()
{
    const QString basePath = QDir(dataRootPath()).filePath(QStringLiteral("logs"));
    QDir dir(basePath);
    dir.mkpath(QStringLiteral("."));
    return dir.filePath(QStringLiteral("pyraqt.log"));
}

void qtMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    if (g_logManager == nullptr) {
        return;
    }

    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:
        g_logManager->info(message);
        break;
    case QtWarningMsg:
        g_logManager->warning(message);
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        g_logManager->error(message);
        break;
    }
}

} // namespace

LogManager::LogManager(QObject *parent)
    : QObject(parent)
    , m_logFilePath(logPath())
{
}

LogManager::~LogManager()
{
    if (g_logManager == this) {
        qInstallMessageHandler(nullptr);
        g_logManager = nullptr;
    }
}

bool LogManager::initialize()
{
    QFile file(m_logFilePath);
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        return false;
    }

    g_logManager = this;
    qInstallMessageHandler(qtMessageHandler);

#if PYRAQT_HAS_SPDLOG
    spdlog::set_level(spdlog::level::info);
#endif

    writeLine(QStringLiteral("INFO"), QStringLiteral("PyraQt logging initialized"));
    return true;
}

QString LogManager::logFilePath() const
{
    return m_logFilePath;
}

void LogManager::info(const QString &message)
{
    writeLine(QStringLiteral("INFO"), message);
}

void LogManager::warning(const QString &message)
{
    writeLine(QStringLiteral("WARN"), message);
}

void LogManager::error(const QString &message)
{
    writeLine(QStringLiteral("ERROR"), message);
}

void LogManager::writeLine(const QString &level, const QString &message)
{
    QFile file(m_logFilePath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [" << level << "] " << message << '\n';
    }

#if PYRAQT_HAS_SPDLOG
    spdlog::info("[{}] {}", level.toStdString(), message.toStdString());
#endif

    emit messageLogged(level, message);
}

} // namespace pyraqt::core

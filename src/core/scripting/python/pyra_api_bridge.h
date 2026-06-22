#pragma once

#include <QObject>
#include <QString>

namespace pyraqt::core {

class LogManager;
class PythonRuntimeManager;

class PyraApiBridge final : public QObject {
    Q_OBJECT

public:
    PyraApiBridge(PythonRuntimeManager &runtimeManager, LogManager &logManager, QObject *parent = nullptr);

    [[nodiscard]] QString bridgeRootPath() const;
    [[nodiscard]] bool ensureBridgeFiles() const;
    [[nodiscard]] QString bootstrapFilePath() const;

signals:
    void statusMessageRequested(const QString &message);
    void dialogMessageRequested(const QString &message);
    void logMessageRequested(const QString &level, const QString &message);

private:
    [[nodiscard]] QString dataRootPath() const;
    [[nodiscard]] QString packageDirectory() const;
    [[nodiscard]] QString bootstrapScript() const;
    [[nodiscard]] QString moduleScript() const;

    PythonRuntimeManager &m_runtimeManager;
    LogManager &m_logManager;
};

} // namespace pyraqt::core

#pragma once

#include <QObject>
#include <QString>

namespace pyraqt::core {

class LogManager final : public QObject {
    Q_OBJECT

public:
    explicit LogManager(QObject *parent = nullptr);
    ~LogManager() override;

    [[nodiscard]] bool initialize();
    [[nodiscard]] QString logFilePath() const;

    void info(const QString &message);
    void warning(const QString &message);
    void error(const QString &message);

signals:
    void messageLogged(const QString &level, const QString &message);

private:
    void writeLine(const QString &level, const QString &message);

    QString m_logFilePath;
};

} // namespace pyraqt::core

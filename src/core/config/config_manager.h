#pragma once

#include <QJsonObject>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariant>

namespace pyraqt::core {

class ConfigManager final : public QObject {
    Q_OBJECT

public:
    explicit ConfigManager(QObject *parent = nullptr);

    [[nodiscard]] QVariant value(const QString &key, const QVariant &fallback = {}) const;
    void setValue(const QString &key, const QVariant &value);

    [[nodiscard]] bool load();
    [[nodiscard]] bool save() const;
    [[nodiscard]] QString configFilePath() const;

signals:
    void valueChanged(const QString &key, const QVariant &value);

private:
    [[nodiscard]] QJsonObject defaults() const;

    QSettings m_settings;
    QJsonObject m_values;
};

} // namespace pyraqt::core

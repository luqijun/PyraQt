#pragma once

#include <QLocale>
#include <QObject>
#include <QStringList>
#include <QTranslator>

class QApplication;

namespace pyraqt::core {

class I18nManager final : public QObject {
    Q_OBJECT

public:
    explicit I18nManager(QApplication &application, QObject *parent = nullptr);

    [[nodiscard]] QString currentLocale() const;
    [[nodiscard]] QStringList availableLocales() const;

public slots:
    bool setLocale(const QString &localeName);

signals:
    void localeChanged(const QString &localeName);

private:
    QApplication &m_application;
    QTranslator m_translator;
    QString m_currentLocale;
};

} // namespace pyraqt::core

#include "core/i18n/i18n_manager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>

namespace pyraqt::core {

I18nManager::I18nManager(QApplication &application, QObject *parent)
    : QObject(parent)
    , m_application(application)
    , m_currentLocale(QStringLiteral("en_US"))
{
}

QString I18nManager::currentLocale() const
{
    return m_currentLocale;
}

QStringList I18nManager::availableLocales() const
{
    return {QStringLiteral("en_US"), QStringLiteral("zh_CN")};
}

bool I18nManager::setLocale(const QString &localeName)
{
    if (!availableLocales().contains(localeName)) {
        return false;
    }

    m_application.removeTranslator(&m_translator);

    const QString translationDir = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("translations"));
    const QString baseName = QStringLiteral("pyraqt_%1").arg(localeName);
    if (!m_translator.load(baseName, translationDir)) {
        if (localeName != QStringLiteral("en_US")) {
            return false;
        }
    } else {
        m_application.installTranslator(&m_translator);
    }

    m_currentLocale = localeName;
    emit localeChanged(localeName);
    return true;
}

} // namespace pyraqt::core

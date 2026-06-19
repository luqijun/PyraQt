#include "core/theme/theme_manager.h"

#include <QApplication>
#include <QFile>
#include <QStringList>

namespace pyraqt::core {

ThemeManager::ThemeManager(QApplication &application, QObject *parent)
    : QObject(parent)
    , m_application(application)
    , m_currentTheme(QStringLiteral("dark"))
{
}

QString ThemeManager::currentTheme() const
{
    return m_currentTheme;
}

QStringList ThemeManager::availableThemes() const
{
    return {QStringLiteral("light"), QStringLiteral("dark")};
}

bool ThemeManager::setTheme(const QString &themeName)
{
    if (!availableThemes().contains(themeName)) {
        return false;
    }

    QFile file(QStringLiteral(":/themes/%1.qss").arg(themeName));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    m_application.setStyleSheet(QString::fromUtf8(file.readAll()));
    m_currentTheme = themeName;
    emit themeChanged(themeName);
    return true;
}

bool ThemeManager::setLightTheme()
{
    return setTheme(QStringLiteral("light"));
}

bool ThemeManager::setDarkTheme()
{
    return setTheme(QStringLiteral("dark"));
}

} // namespace pyraqt::core

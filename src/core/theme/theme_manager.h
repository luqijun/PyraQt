#pragma once

#include <QObject>
#include <QString>

class QApplication;

namespace pyraqt::core {

class ThemeManager final : public QObject {
    Q_OBJECT

public:
    explicit ThemeManager(QApplication &application, QObject *parent = nullptr);

    [[nodiscard]] QString currentTheme() const;
    [[nodiscard]] QStringList availableThemes() const;

public slots:
    bool setTheme(const QString &themeName);
    bool setLightTheme();
    bool setDarkTheme();

signals:
    void themeChanged(const QString &themeName);

private:
    QApplication &m_application;
    QString m_currentTheme;
};

} // namespace pyraqt::core
